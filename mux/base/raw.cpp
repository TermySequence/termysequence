// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "raw.h"
#include "output.h"
#include "connwatch.h"
#include "listener.h"
#include "exception.h"
#include "os/time.h"
#include "os/logging.h"
#include "lib/handshake.h"
#include "lib/protocol.h"
#include "lib/attrstr.h"

#include <unistd.h>

RawInstance::RawInstance(int protocolType, int fd, const char *buf, size_t len) :
    ConnInstance("conn", false),
    m_protocolType(protocolType),
    m_newFd(-1),
    m_indirect(false)
{
    m_fd = fd;
    m_id.generate();

    m_attributes[Tsq::attr_ID] = m_id.str();
    m_attributes[Tsq::attr_STARTED] = std::to_string(osWalltime());

    m_machine = new ClientMachine(this, buf, len);
}

RawInstance::RawInstance(int protocolType, int fd, int newFd) :
    ConnInstance("conn", false),
    m_protocolType(protocolType),
    m_newFd(newFd),
    m_indirect(true)
{
    m_fd = fd;
    m_id.generate();

    m_attributes[Tsq::attr_ID] = m_id.str();
    m_attributes[Tsq::attr_STARTED] = std::to_string(osWalltime());

    m_machine = new ClientMachine(this, nullptr, 0);
}

/*
 * This thread
 */
bool
RawInstance::setRemoteId(const Tsq::Uuid &remoteId)
{
    if (g_listener->checkServer(m_remoteId = remoteId)) {
        m_attributes[Tsq::attr_PEER] = remoteId.str();
        return true;
    }

    return false;
}

void
RawInstance::writeResponse(const char *buf, size_t len)
{
    m_output->writeFd(m_fd, buf, len);
    if (m_indirect)
        m_output->writeFd(m_newFd, buf, len);
}

bool
RawInstance::setMachine(Tsq::ProtocolMachine *newMachine, StringMap &attributes)
{
    delete m_machine;
    m_machine = newMachine;

    if (m_indirect) {
        // Send the connector our id
        m_output->writeFd(m_fd, m_id.buf, 16);
        close(m_fd);
        setfd(m_newFd);
        m_newFd = -1;
    }

    m_attributes.merge(attributes);

    auto i = m_attributes.find(Tsq::attr_COMMAND_KEEPALIVE);
    if (i != m_attributes.end())
        setKeepalive(atoi(i->second.c_str()), 2);

    if (!m_machine->start())
        return false;

    m_haveConnection = true;
    m_output->start(-1);
    g_listener->sendWork(ListenerConfirmTerm, this);
    return true;
}

bool
RawInstance::handleFd()
{
    try {
        if (m_machine->connRead(m_fd))
            return true;
    }
    catch (const TsqException &e) {
        LOGERR("Conn %p: %s\n", this, e.what());
    }

    return handleClose(0, TSQ_STATUS_LOST_CONN);
}

void
RawInstance::postConnect()
{
    if (m_protocolType == TSQ_PROTOCOL_RAW)
        pushChannelTest();
    if (m_timeout != -1)
        pushConfigureKeepalive(m_timeout / 2);
}

void
RawInstance::postDisconnect(bool)
{
}

bool
RawInstance::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case TermClose:
    case TermDisconnect:
        return handleClose(DisActive, item.value);
    case TermWatchAdded:
        // do nothing
        break;
    case TermWatchReleased:
        return handleWatchReleased((ConnWatch*)item.value);
    case TermServerReleased:
        return handleServerReleased((ServerProxy*)item.value);
    case TermProxyReleased:
        handleProxyReleased((TermProxy*)item.value);
        break;
    default:
        break;
    }

    return true;
}

bool
RawInstance::handleIdle()
{
    LOGDBG("Conn %p: keepalive timed out\n", this);
    return handleClose(DisActive, TSQ_STATUS_IDLE_TIMEOUT);
}

void
RawInstance::threadMain()
{
    try {
        if (m_indirect)
            writeFd("", 1);
        if (m_machine->start())
            runDescriptorLoop();
    }
    catch (const TsqException &e) {
        LOGERR("Conn %p: %s\n", this, e.what());
    } catch (const std::exception &e) {
        LOGERR("Conn %p: caught exception: %s\n", this, e.what());
    }

    closefd();
    if (m_newFd != -1)
        close(m_newFd);
    LOGDBG("Conn %p: goodbye\n", this);

    g_listener->sendWork(ListenerRemoveTerm, this);
}
