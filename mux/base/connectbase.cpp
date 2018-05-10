// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "connectbase.h"
#include "os/conn.h"
#include "lib/protocol.h"
#include "config.h"

#include <cerrno>
#include <unistd.h>

void
ConnectorBase::setError(Tsq::ConnectTaskError type, bool noerrno, int errnum)
{
    *m_exitStatusPtr = 1;
    m_errtype = type;
    m_errnum = errnum;
    if (noerrno)
        errno = 0;
}

bool
ConnectorBase::setHandshakeReadError(int rc)
{
    if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR) {
            return true;
        }
        if (errno != EIO) {
            setError(Tsq::ConnectTaskErrorRemoteReadFailed);
            return false;
        }
    }

    setError(Tsq::ConnectTaskErrorRemoteConnectFailed, true);
    return false;
}

void
ConnectorBase::setAttribute(const std::string &key, const std::string &val)
{
    m_attributes.append(key);
    m_attributes.push_back('\0');
    m_attributes.append(val);
    m_attributes.push_back('\0');
}

int
ConnectorBase::writeData(int fd, std::string &data)
{
    size_t sent = 0, len = data.size();
    const char *buf = data.data();

    while (sent < len) {
        ssize_t rc = write(fd, buf + sent, len - sent);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                data.erase(0, sent);
                return 0;
            }

            setError(Tsq::ConnectTaskErrorWriteFailed);
            return -1;
        }
        sent += rc;
    }

    data.clear();
    return 1;
}

bool
ConnectorBase::readLocalHandshake()
{
    char buf[256];

    ssize_t rc = read(m_sd, buf, sizeof(buf));
    if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            return true;

        setError(Tsq::ConnectTaskErrorLocalReadFailed);
        return false;
    }
    if (rc == 0) {
        setError(Tsq::ConnectTaskErrorLocalConnectFailed, true);
        return false;
    }

    int rc2 = m_handshake.processHello(buf, rc);
    if (rc2 == Tsq::ShakeOngoing)
        return true;
    if (rc2 != Tsq::ShakeSuccess) {
        setError(Tsq::ConnectTaskErrorLocalHandshakeFailed, true, rc2);
        return false;
    }

    m_outbuf = m_handshake.getResponse(CLIENT_VERSION, m_protocolType);
    m_state = WritingLocalHandshake;
    (*m_fdp)[1].events = POLLOUT;
    return true;
}

bool
ConnectorBase::writeLocalHandshake()
{
    switch (writeData(m_sd, m_outbuf)) {
    case 1:
        m_state = ReadingLocalByte1;
        (*m_fdp)[1].events = POLLIN;
        // fallthru
    case 0:
        return true;
    default:
        return false;
    }
}

bool
ConnectorBase::readLocalByte(int nextState)
{
    char c;

    ssize_t rc = read(m_sd, &c, 1);
    if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            return true;

        setError(Tsq::ConnectTaskErrorLocalTransferFailed);
        return false;
    }
    if (rc == 0 || c) {
        setError(Tsq::ConnectTaskErrorLocalTransferFailed, true);
        return false;
    }

    m_state = nextState;
    (*m_fdp)[1].events = POLLOUT;
    return true;
}

bool
ConnectorBase::writeDescriptor(int sendFd)
{
    if (osLocalSendFd(m_sd, &sendFd, 1) < 0) {
        if (errno == EAGAIN || errno == EINTR)
            return true;

        setError(Tsq::ConnectTaskErrorLocalTransferFailed);
        return false;
    }

    m_state = ReadingLocalByte2;
    (*m_fdp)[1].events = POLLIN;
    return true;
}

bool
ConnectorBase::writeHello()
{
    switch (writeData(m_sd, m_hello)) {
    case 1:
        m_state = ReadingLocalResponse;
        (*m_fdp)[1].events = POLLIN;
        // fallthru
    case 0:
        return true;
    default:
        return false;
    }
}

bool
ConnectorBase::readLocalResponse()
{
    char buf[256];

    ssize_t rc = read(m_sd, buf, sizeof(buf));
    if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            return true;

        setError(Tsq::ConnectTaskErrorLocalTransferFailed);
    }
    if (rc == 0) {
        setError(Tsq::ConnectTaskErrorLocalTransferFailed, true);
        return false;
    }

    int rc2 = m_response.processResponse(buf, rc);
    switch (rc2) {
    case Tsq::ShakeOngoing:
        return true;
    case Tsq::ShakeSuccess:
        switch (m_response.protocolType) {
        case TSQ_PROTOCOL_TERM:
        case TSQ_PROTOCOL_RAW:
            break;
        case TSQ_PROTOCOL_REJECT:
            setError(Tsq::ConnectTaskErrorLocalRejection, true, m_response.clientVersion);
            return false;
        default:
            setError(Tsq::ConnectTaskErrorLocalBadProtocol, true, m_response.protocolType);
            return false;
        }
        break;
    default:
        setError(Tsq::ConnectTaskErrorLocalBadResponse, true, rc2);
        return false;
    }

    m_state = WritingAttributes;
    (*m_fdp)[1].events = POLLOUT;
    return true;
}

bool
ConnectorBase::writeAttributes()
{
    switch (writeData(m_sd, m_attributes)) {
    case 1:
        m_state = ReadingConnId;
        (*m_fdp)[1].events = POLLIN;
        // fallthru
    case 0:
        return true;
    default:
        return false;
    }
}

bool
ConnectorBase::readConnId()
{
    ssize_t rc = read(m_sd, m_connId, 16 - m_connLen);
    if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            return true;

        setError(Tsq::ConnectTaskErrorReadIdFailed);
        return false;
    }
    if (rc == 0) {
        setError(Tsq::ConnectTaskErrorReadIdFailed, true);
        return false;
    }

    return (m_connLen += rc) < 16;
}

bool
ConnectorBase::processConnectorFd(pollfd &pfd, int sendFd)
{
    switch (m_state) {
    case ReadingRemoteHandshake:
        return readRemoteHandshake(pfd);
    case WritingRemoteHandshake:
        return writeRemoteHandshake(pfd);
    case ReadingLocalHandshake:
        return readLocalHandshake();
    case WritingLocalHandshake:
        return writeLocalHandshake();
    case ReadingLocalByte1:
        return readLocalByte(WritingDescriptor);
    case WritingDescriptor:
        return writeDescriptor(sendFd);
    case ReadingLocalByte2:
        return readLocalByte(WritingHello);
    case WritingHello:
        return writeHello();
    case ReadingLocalResponse:
        return readLocalResponse();
    case WritingAttributes:
        return writeAttributes();
    default:
        return readConnId();
    }
}
