// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "connecttask.h"
#include "listener.h"
#include "zombies.h"
#include "status.h"
#include "exception.h"
#include "parsemap.h"
#include "os/process.h"
#include "os/pty.h"
#include "os/dir.h"
#include "os/conn.h"
#include "os/signal.h"
#include "os/logging.h"
#include "lib/attrstr.h"
#include "lib/wire.h"
#include "lib/protocol.h"
#include "config.h"

#include <unistd.h>

RunConnect::RunConnect(Tsq::ProtocolUnmarshaler *unm) :
    TaskBase("connect", unm, ThreadBaseFd)
{
    parseStringMap(*unm, m_params);

    m_exitStatusPtr = &m_exitStatus;
    m_fdp = &m_fds;
    m_timeout = -1;
}

/*
 * This thread
 */
inline void
RunConnect::reportStarting()
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumberPair(Tsq::TaskStarting, m_pid);
    g_listener->forwardToClient(m_clientId, m.result());
}

void
RunConnect::reportStatus(Tsq::TaskStatus status, const char *buf, unsigned len)
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumber(status);
    m.addBytes(buf, len);
    g_listener->forwardToClient(m_clientId, m.result());
}

void
RunConnect::reportError()
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumberPair(Tsq::TaskError, m_errtype);
    m.addNumber(m_errnum);
    m.addString(errno ? strerror(errno) : "");
    g_listener->forwardToClient(m_clientId, m.result());
}

inline void
RunConnect::handleCancel()
{
    osKillProcess(m_pid);
    LOGDBG("RunConnect %p: sent SIGTERM to pid %d (canceling)\n", this, m_pid);
    m_exitStatus = 2;
}

bool
RunConnect::handleData(std::string *data)
{
    {
        Lock lock(this);
        m_incomingData.erase(data);
    }

    if (m_state == ReadingRemoteHandshake) {
        if (m_readCount >= CONNECT_HANDSHAKE_MAX) {
            setError(Tsq::ConnectTaskErrorRemoteLimitExceeded);
            return false;
        }
        m_readCount += data->size();
        m_outbuf = std::move(*data);
        m_state = WritingRemoteHandshake;
        m_fds.back().events = POLLOUT;
    }

    delete data;
    return true;
}

bool
RunConnect::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case TaskClose:
        handleCancel();
        return false;
    case TaskInput:
        return handleData((std::string *)item.value);
    }

    return true;
}

bool
RunConnect::handleInterrupt()
{
    // Wait for explicit TaskClose
    return true;
}

void
RunConnect::localConnect()
{
    // Place terminal in raw mode
    if (m_pty)
        osMakeRawTerminal(m_fd);

    // Collect process attributes
    TermStatusTracker status;
    status.updateOnce(m_fd, m_pid, SERVER_NAME);
    m_attributes.append(4, '\0');

    for (const auto &i: status.changedMap()) {
        setAttribute(i.first, i.second);
    }

    setAttribute(Tsq::attr_SENDER_ID, m_clientId.str());

    // Finish out attributes
    auto i = m_params.find(Tsq::attr_COMMAND_KEEPALIVE);
    if (i != m_params.end() && atoi(i->second.c_str()) > 0) {
        setAttribute(Tsq::attr_COMMAND_KEEPALIVE, i->second);
    }
    uint32_t len = htole32(m_attributes.size() - 4);
    m_attributes.replace(0, 4, reinterpret_cast<char*>(&len), 4);

    // Make local connection
    int sd[2];
    osSocketPair(sd, true);
    m_sd = sd[0];
    g_listener->sendWork(ListenerAddReader, sd[1]);

    m_handshake.reset(true);
    m_state = ReadingLocalHandshake;
    m_fds[1].fd = m_sd;
}

bool
RunConnect::readRemoteHandshake(pollfd &)
{
    char buf[1024];

    ssize_t rc = read(m_fd, buf, sizeof(buf));
    if (rc <= 0)
        return setHandshakeReadError(rc);

    for (size_t i = 0; i < rc; ++i)
    {
        int rc2 = m_handshake.processHelloByte(buf[i]);
        switch (rc2) {
        case Tsq::ShakeOngoing:
            continue;
        case Tsq::ShakeSuccess:
            m_hello = m_handshake.getHello();
            localConnect();
            return true;
        case Tsq::ShakeBadLeadingContent:
            m_outbuf.append(m_handshake.leadingContent, m_handshake.leadingContentLen);
            continue;
        default:
            setError(Tsq::ConnectTaskErrorRemoteHandshakeFailed, true, rc2);
            return false;
        }
    }
    if (!m_outbuf.empty()) {
        reportStatus(Tsq::TaskRunning, m_outbuf.data(), m_outbuf.size());
    }
    return true;
}

bool
RunConnect::writeRemoteHandshake(pollfd &)
{
    switch (writeData(m_fd, m_outbuf)) {
    case 1:
        m_state = ReadingRemoteHandshake;
        m_fds[1].events = POLLIN;
        // fallthru
    case 0:
        return true;
    default:
        return false;
    }
}

bool
RunConnect::handleMultiFd(pollfd &pfd)
{
    return processConnectorFd(pfd, m_fd);
}

bool
RunConnect::begin()
{
    PtyParams params{};
    params.command = m_params[Tsq::attr_COMMAND_COMMAND];
    params.env = m_params[Tsq::attr_COMMAND_ENVIRON];
    params.dir = m_params[Tsq::attr_COMMAND_STARTDIR];

    for (int i = 0; i < params.command.size(); ++i)
        if (params.command[i] == '\x1f')
            params.command[i] = '\0';

    for (int i = 0; i < params.env.size(); ++i)
        if (params.env[i] == '\x1f')
            params.env[i] = '\0';

    osRelativeToHome(params.dir);
    params.daemon = true;

    auto i = m_params.find(Tsq::attr_COMMAND_PROTOCOL);
    m_protocolType = (i != m_params.end() && i->second == "1") ?
        TSQ_PROTOCOL_RAW_SERVERFD : TSQ_PROTOCOL_TERM_SERVERFD;
    i = m_params.find(Tsq::attr_COMMAND_PTY);
    m_pty = (i != m_params.end() && i->second == "1");

    try {
        if (m_pty) {
            params.env.append("+TERM=dumb");
            m_fd = osForkTerminal(params, &m_pid);
        } else {
            params.env.append("-TERM");
            m_fd = osForkProcess(params, &m_pid);
        }
    }
    catch (const ErrnoException &) {
        LOGDBG("RunConnect %p: %m\n", this);
        return false;
    }

    LOGDBG("RunConnect %p: started process (pid %d)\n", this, m_pid);
    g_reaper->ignoreProcess(m_pid);
    reportStarting();
    loadfd();
    return true;
}

void
RunConnect::threadMain()
{
    try {
        if (begin())
            runDescriptorLoopMulti();

        switch (m_exitStatus) {
        case 0:
            LOGDBG("RunConnect %p: succeeded (id %s)\n", this,
                   Tsq::Uuid(m_connId).str().c_str());
            reportStatus(Tsq::TaskFinished, m_connId, 16);
            break;
        case 1:
            LOGDBG("RunConnect %p: error code=%d, sub=%d, errno=%d\n", this,
                   m_errtype, m_errnum, errno);
            reportError();
        }
    }
    catch (const std::exception &e) {
        LOGERR("RunConnect %p: caught exception: %s\n", this, e.what());
    }

    closefd();
    if (m_sd != -1)
        close(m_sd);

    g_listener->sendWork(ListenerRemoveTask, this);
}
