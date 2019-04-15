// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "runtask.h"
#include "listener.h"
#include "zombies.h"
#include "status.h"
#include "exception.h"
#include "parsemap.h"
#include "os/process.h"
#include "os/dir.h"
#include "os/signal.h"
#include "os/logging.h"
#include "lib/attrstr.h"
#include "lib/wire.h"
#include "lib/protocol.h"
#include "lib/enums.h"
#include "config.h"

#include <sys/socket.h>
#include <unistd.h>

#define HEADERSIZE 60

RunCommand::RunCommand(Tsq::ProtocolUnmarshaler *unm) :
    TaskBase("command", unm, ThreadBaseFd)
{
    m_chunkSize = unm->parseNumber();
    m_windowSize = unm->parseNumber();
    parseStringMap(*unm, m_params);

    uint32_t command = htole32(TSQ_TASK_OUTPUT);
    uint32_t status = htole32(Tsq::TaskRunning);

    m_buf = new char[m_chunkSize + HEADERSIZE];
    memcpy(m_buf, &command, 4);
    memcpy(m_buf + 8, m_clientId.buf, 16);
    memcpy(m_buf + 24, g_listener->id().buf, 16);
    memcpy(m_buf + 40, m_taskId.buf, 16);
    memcpy(m_buf + 56, &status, 4);
    m_ptr = m_buf + HEADERSIZE;

    m_timeout = -1;
}

RunCommand::~RunCommand()
{
    delete [] m_buf;
}

/*
 * This thread
 */
void
RunCommand::reportStarting()
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumberPair(Tsq::TaskStarting, m_pid);
    g_listener->forwardToClient(m_clientId, m.result());
}

inline void
RunCommand::reportAck()
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumber(Tsq::TaskAcking);
    m.addNumber64(m_received);
    g_listener->forwardToClient(m_clientId, m.result());
}

void
RunCommand::reportFinished()
{
    TermStatusTracker status;
    status.setOutcome(m_pid, m_disposition);
    LOGDBG("RunCommand %p: %s\n", this, status.outcomeStr());

    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumber(status.outcome() == Tsq::TermExit0 ? Tsq::TaskFinished : Tsq::TaskError);
    m.addNumberPair(status.outcome(), status.exitcode());
    m.addString(status.outcomeStr());
    g_listener->forwardToClient(m_clientId, m.result());
}

void
RunCommand::reportError(int errtype, int errnum)
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumber(Tsq::TaskError);
    m.addNumberPair(Tsq::TermRunning, errtype);
    m.addString(strerror(errnum));
    g_listener->forwardToClient(m_clientId, m.result());
}

void
RunCommand::handleData(std::string *data)
{
    Tsq::ProtocolUnmarshaler unm(data->data(), data->size());

    switch (unm.parseNumber()) {
    case Tsq::TaskAcking:
        m_acked = unm.parseNumber64();
        m_running = true;
        if (!m_throttled)
            m_fds[1].events = POLLIN;
        break;
    case Tsq::TaskRunning:
        if (m_fds[2].fd != -1) {
            m_outdata.push(data);
            m_fds[2].events = POLLOUT;
            // Keep data in incoming pool until written
            return;
        }
        break;
    }
    {
        Lock lock(this);
        m_incomingData.erase(data);
    }
    delete data;
}

inline void
RunCommand::handleWrite(pollfd &pfd)
{
    if (m_outdata.empty()) {
        pfd.events = 0;
        return;
    }

    std::string *data = m_outdata.front();
    size_t len = data->size() - 4;
    ssize_t rc;

    if (len == 0) {
        LOGDBG("RunCommand %p: remote EOF\n", this);
        goto eof;
    }
    else if ((rc = write(m_fd, data->data() + 4, len)) == len) {
        {
            Lock lock(this);
            m_incomingData.erase(data);
        }
        m_outdata.pop();
        delete data;
    }
    else if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            return;

        LOGNOT("RunCommand %p: write failed: %m\n", this);
        goto eof;
    }
    else {
        data->erase(4, rc);
    }

    m_received += rc;
    if (m_chunks < m_received / m_chunkSize) {
        m_chunks = m_received / m_chunkSize;
        reportAck();
    }
    return;
eof:
    // Stop writing, rely on incoming pool to clean up data strings
    shutdown(m_fd, SHUT_WR);
    pfd.fd = -1;
}

inline bool
RunCommand::handleRead(pollfd &pfd)
{
    if (m_sent - m_acked >= m_windowSize * m_chunkSize) {
        // Stop and wait for ack message
        m_running = false;
        pfd.events = 0;
        return true;
    }

    ssize_t rc = read(m_fd, m_ptr, m_chunkSize);
    if (rc > 0) {
        uint32_t length = htole32(HEADERSIZE - 8 + rc);
        memcpy(m_buf + 4, &length, 4);

        std::string buf(m_buf, HEADERSIZE + rc);
        m_sent += rc;

        if (!throttledOutput(buf)) {
            LOGDBG("RunCommand %p: throttled (local)\n", this);
            m_throttled = true;
            pfd.events = 0;
        }
        return true;
    }
    else if (rc < 0) {
        LOGNOT("RunCommand %p: read failed: %m\n", this);
    }
    else {
        // Note: no zero-length EOF message
        LOGDBG("RunCommand %p: local EOF\n", this);
    }

    m_fds[2] = m_fds[1] = pollfd{ -1 };
    closefd();
    if (m_exited) {
        reportFinished();
        return false;
    }
    return true;
}

bool
RunCommand::handleMultiFd(pollfd &pfd)
{
    if (pfd.events != POLLOUT) {
        return handleRead(pfd);
    } else {
        handleWrite(pfd);
        return true;
    }
}

void
RunCommand::handleCancel()
{
    osKillProcess(m_pid);
    g_reaper->abandonProcess(m_pid);
    LOGDBG("RunCommand %p: sent SIGTERM to pid %d (canceling)\n", this, m_pid);
    LOGDBG("RunCommand %p: abandoned pid %d\n", this, m_pid);
}

bool
RunCommand::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case TaskClose:
        handleCancel();
        return false;
    case TaskInput:
        handleData((std::string *)item.value);
        return true;
    case TaskPause:
        m_throttled = true;
        m_fds[1].events = 0;
        LOGDBG("RunCommand %p: throttled (remote)\n", this);
        break;
    case TaskResume:
        m_throttled = false;
        if (m_running)
            m_fds[1].events = POLLIN;
        LOGDBG("RunCommand %p: resumed\n", this);
        break;
    case TaskProcessExited:
        m_disposition = item.value;
        m_exited = true;
        if (m_fd == -1) {
            reportFinished();
            return false;
        }
        break;
    }

    return true;
}

bool
RunCommand::handleInterrupt()
{
    m_fds[2] = m_fds[1] = pollfd{ -1 };
    closefd();
    // Wait for explicit TaskClose
    return true;
}

bool
RunCommand::begin()
{
    ForkParams params;
    params.command = m_params[Tsq::attr_COMMAND_COMMAND];
    params.env = m_params[Tsq::attr_COMMAND_ENVIRON];
    params.dir = m_params[Tsq::attr_COMMAND_STARTDIR];

    for (char &c: params.command)
        if (c == '\x1f')
            c = '\0';

    for (char &c: params.env)
        if (c == '\x1f')
            c = '\0';

    osRelativeToHome(params.dir);
    params.daemon = false;
    params.devnull = !m_chunkSize;

    try {
        m_fd = osForkProcess(params, &m_pid);
    }
    catch (const ErrnoException &) {
        LOGDBG("RunCommand %p: %m\n", this);
        return false;
    }

    m_fds[1] = pollfd{ m_fd };
    m_fds.emplace_back(pollfd{ m_fd });

    LOGDBG("RunCommand %p: started process (pid %d)\n", this, m_pid);
    g_reaper->registerProcess(this, m_pid);
    reportStarting();
    return true;
}

void
RunCommand::threadMain()
{
    try {
        if (begin())
            runDescriptorLoopMulti();
    }
    catch (const std::exception &e) {
        LOGERR("RunCommand %p: caught exception: %s\n", this, e.what());
    }

    closefd();
    g_listener->sendWork(ListenerRemoveTask, this);
}
