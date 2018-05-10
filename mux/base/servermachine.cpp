// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "servermachine.h"
#include "reader.h"
#include "listener.h"
#include "exception.h"
#include "os/conn.h"
#include "os/logging.h"
#include "lib/protocol.h"
#include "lib/handshake.h"
#include "lib/raw.h"
#include "lib/term.h"
#include "config.h"

#include <unistd.h>

ServerMachine::ServerMachine(TermReader *parent) :
    Tsq::ProtocolMachine(parent),
    m_parent(parent),
    m_expectingFd(false),
    m_gotFd(false),
    m_expectingConn(false)
{
    m_handshake = new Tsq::ServerHandshake(g_listener->id().buf);
}

ServerMachine::~ServerMachine()
{
    delete m_handshake;
}

bool
ServerMachine::receiveFd(int fd)
{
    m_expectingFd = false;
    m_gotFd = true;

    if (m_expectingConn) {
        int newFd;
        osLocalReceiveFd(fd, &newFd, 1);
        m_parent->createConn(m_connType, newFd);
        return false;
    } else {
        int newFd[2];
        osLocalReceiveFd(fd, newFd, 2);
        m_parent->setFd(newFd[0], newFd[1]);
        m_handshake->reset();
        return start();
    }
}

bool
ServerMachine::connRead(const char *buf, size_t len)
{
    Tsq::ProtocolMachine *newMachine;

    switch (m_handshake->processResponse(buf, len)) {
    case Tsq::ShakeOngoing:
        // keep reading more bytes
        return true;
    case Tsq::ShakeSuccess:
        buf = m_handshake->residualBuf;
        len = m_handshake->residualLen;
        break;
    default:
        throw ProtocolException(); // Received bad handshake message from client
    }

    switch (m_handshake->protocolType) {
    case TSQ_PROTOCOL_REJECT:
        // Close requested - reason is in clientVersion
        m_parent->m_exitStatus = m_handshake->clientVersion;
        return false;
    case TSQ_PROTOCOL_TERM:
        m_parent->m_remoteId = Tsq::Uuid(m_handshake->clientId);
        newMachine = new Tsq::TermProtocol(m_parent, buf, len);
        return m_parent->setMachine(newMachine, -TSQ_PROTOCOL_TERM);
    case TSQ_PROTOCOL_RAW:
        m_parent->m_remoteId = Tsq::Uuid(m_handshake->clientId);
        newMachine = new Tsq::RawProtocol(m_parent, buf, len);
        return m_parent->setMachine(newMachine, -TSQ_PROTOCOL_RAW);
    case TSQ_PROTOCOL_CLIENTFD:
        if (len) {
            LOGNOT("Reader %p: unexpected data after transfer fd request\n", m_parent);
            m_parent->m_exitStatus = TSQ_STATUS_PROTOCOL_ERROR;
            return false;
        }
        if (m_gotFd) {
            LOGNOT("Reader %p: too many transfer fd requests\n", m_parent);
            m_parent->m_exitStatus = TSQ_STATUS_PROTOCOL_ERROR;
            return false;
        }
        m_expectingFd = true;
        m_parent->writeFd("", 1);
        return true;
    case TSQ_PROTOCOL_TERM_SERVER:
        m_parent->createConn(TSQ_PROTOCOL_TERM, buf, len);
        return false;
    case TSQ_PROTOCOL_RAW_SERVER:
        m_parent->createConn(TSQ_PROTOCOL_RAW, buf, len);
        return false;
    case TSQ_PROTOCOL_TERM_SERVERFD:
    case TSQ_PROTOCOL_RAW_SERVERFD:
        if (len) {
            LOGNOT("Reader %p: unexpected data after transfer fd request\n", m_parent);
            m_parent->m_exitStatus = TSQ_STATUS_PROTOCOL_ERROR;
            return false;
        }
        if (m_gotFd) {
            LOGNOT("Reader %p: too many transfer fd requests\n", m_parent);
            m_parent->m_exitStatus = TSQ_STATUS_PROTOCOL_ERROR;
            return false;
        }
        m_expectingFd = m_expectingConn = true;
        m_connType = m_handshake->protocolType == TSQ_PROTOCOL_TERM_SERVERFD ?
            TSQ_PROTOCOL_TERM : TSQ_PROTOCOL_RAW;
        m_parent->writeFd("", 1);
        return true;
    default:
        LOGNOT("Reader %p: unsupported protocol type %u\n", m_parent, m_handshake->protocolType);
        m_parent->m_exitStatus = TSQ_STATUS_PROTOCOL_MISMATCH;
        return false;
    }
}

bool
ServerMachine::start()
{
    std::string hello = m_handshake->getHello(SERVER_VERSION, TSQ_PROTOCOL_VERSION);
    m_parent->writeFd(hello.data(), hello.size());
    return true;
}

bool
ServerMachine::connRead(int fd)
{
    ssize_t got;
    char buf[READER_BUFSIZE];

    if (m_expectingFd)
        return receiveFd(fd);

    got = read(fd, buf, sizeof(buf));
    if (got < 0) {
        if (errno == EINTR)
            return true;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;

        LOGDBG("Reader %p: read failed: %s\n", m_parent, strerror(errno));
        return false;
    }
    if (got == 0) {
        LOGDBG("Reader %p: connection closed during handshake\n", m_parent);
        return false;
    }

    // feed the machine
    return connRead(buf, got);
}
