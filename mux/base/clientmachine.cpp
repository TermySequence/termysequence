// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "clientmachine.h"
#include "raw.h"
#include "exception.h"
#include "parsemap.h"
#include "os/logging.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/handshake.h"
#include "lib/term.h"
#include "lib/raw.h"
#include "config.h"

#include <unistd.h>

ClientMachine::ClientMachine(RawInstance *parent, const char *buf, size_t len) :
    Tsq::ProtocolMachine(parent, buf, len),
    m_parent(parent)
{
    m_handshake = new Tsq::ClientHandshake(parent->id().buf, true);
}

ClientMachine::~ClientMachine()
{
    delete m_handshake;
}

bool
ClientMachine::finish()
{
    StringMap map;
    Tsq::ProtocolUnmarshaler unm(m_attributes.data(), m_length);

    parseUtf8Map(unm, map);

    if (m_parent->m_indirect) {
        m_attributes.clear();
    } else {
        m_attributes.erase(0, m_length);
    }

    const char *buf = m_attributes.data();
    size_t len = m_attributes.size();
    Tsq::ProtocolMachine *newMachine;

    if (m_parent->m_protocolType == TSQ_PROTOCOL_TERM) {
        newMachine = new Tsq::TermProtocol(m_parent, buf, len);
    } else {
        newMachine = new Tsq::RawProtocol(m_parent, buf, len);
    }

    return m_parent->setMachine(newMachine, map);
}

bool
ClientMachine::handshakeRead(const char *buf, size_t len)
{
    int protocolType = TSQ_PROTOCOL_REJECT;
    unsigned clientVersion = TSQ_STATUS_DUPLICATE_CONN;

    switch (m_handshake->processHello(buf, len)) {
    case Tsq::ShakeOngoing:
        // keep reading more bytes
        return true;
    case Tsq::ShakeSuccess:
        break;
    default:
        throw ProtocolException(); // Received bad handshake message from server
    }

    if (m_handshake->protocolVersion != TSQ_PROTOCOL_VERSION) {
        LOGERR("Term %p: Unsupported server protocol version %u\n",
               m_parent, m_handshake->protocolVersion);
        clientVersion = TSQ_STATUS_PROTOCOL_MISMATCH;
    }
    else if (m_parent->setRemoteId(m_handshake->serverId)) {
        protocolType = m_parent->m_protocolType;
        clientVersion = CLIENT_VERSION;
    }

    auto response = m_handshake->getResponse(clientVersion, protocolType);
    m_parent->writeResponse(response.data(), response.size());

    switch (protocolType) {
    case TSQ_PROTOCOL_TERM:
    case TSQ_PROTOCOL_RAW:
        m_handshaking = false;
        return connRead(m_handshake->residualBuf, m_handshake->residualLen);
    default:
        return false;
    }
}

bool
ClientMachine::connRead(const char *buf, size_t len)
{
    if (m_handshaking)
        return handshakeRead(buf, len);

    m_attributes.append(buf, len);

    if (m_needLength && m_attributes.size() >= 4) {
        memcpy(&m_length, m_attributes.data(), 4);
        m_length = le32toh(m_length);
        m_attributes.erase(0, 4);
        m_needLength = false;
    }
    if (!m_needLength && m_attributes.size() >= m_length) {
        // attribute reading finished
        return finish();
    }

    return true;
}

bool
ClientMachine::connRead(int fd)
{
    ssize_t got;
    char buf[READER_BUFSIZE];

    got = read(fd, buf, sizeof(buf));
    if (got < 0) {
        if (errno == EINTR)
            return true;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;

        LOGDBG("Term %p: read failed: %s\n", m_parent, strerror(errno));
        return false;
    }
    if (got == 0) {
        LOGDBG("Term %p: connection closed during handshake\n", m_parent);
        return false;
    }

    // feed the machine
    return connRead(buf, got);
}
