// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "downloadtask.h"
#include "listener.h"
#include "exception.h"
#include "os/fd.h"
#include "os/dir.h"
#include "os/logging.h"
#include "lib/wire.h"
#include "lib/protocol.h"

#include <unistd.h>

#define HEADERSIZE 60

//
// Download from file
//
void
FileDownload::setup(Tsq::ProtocolUnmarshaler *unm)
{
    m_chunkSize = unm->parseNumber();
    m_windowSize = unm->parseNumber();

    uint32_t command = htole32(TSQ_TASK_OUTPUT);
    uint32_t status = htole32(Tsq::TaskRunning);

    m_buf = new char[m_chunkSize + HEADERSIZE];
    memcpy(m_buf, &command, 4);
    memcpy(m_buf + 8, m_clientId.buf, 16);
    memcpy(m_buf + 24, g_listener->id().buf, 16);
    memcpy(m_buf + 40, m_taskId.buf, 16);
    memcpy(m_buf + 56, &status, 4);
    m_ptr = m_buf + HEADERSIZE;
}

FileDownload::FileDownload(Tsq::ProtocolUnmarshaler *unm) :
    TaskBase("download", unm, TaskBaseThrottlable|ThreadBaseFd)
{
    setup(unm);
    m_targetName = unm->parseString();
}

FileDownload::FileDownload(Tsq::ProtocolUnmarshaler *unm, unsigned flags) :
    TaskBase("pipein", unm, flags|TaskBaseThrottlable|ThreadBaseFd)
{
    setup(unm);
    m_timeout = -1;
}

FileDownload::~FileDownload()
{
    delete [] m_buf;
}

/*
 * This thread
 */
void
FileDownload::handleData(std::string *data)
{
    {
        Lock lock(this);
        m_incomingData.erase(data);
    }

    Tsq::ProtocolUnmarshaler unm(data->data(), data->size());
    m_acked = unm.parseNumber64();
    delete data;

    if (!m_running) {
        m_running = true;
        enablefd(!m_throttled);
    }
}

bool
FileDownload::handleFd()
{
    if (m_sent - m_acked >= m_windowSize * m_chunkSize) {
        // Stop and wait for ack message
        m_running = false;
        enablefd(false);
    }
    else {
        ssize_t rc = read(m_fd, m_ptr, m_chunkSize);
        if (rc < 0) {
            reportError(Tsq::DownloadTaskErrorReadFailed, strerror(errno));
            return false;
        }

        uint32_t length = htole32(HEADERSIZE - 8 + rc);
        memcpy(m_buf + 4, &length, 4);

        std::string buf(m_buf, HEADERSIZE + rc);
        m_sent += rc;

        if (!throttledOutput(buf)) {
            LOGDBG("Download %p: throttled (local)\n", this);
            m_throttled = true;
            enablefd(false);
        }
        if (rc == 0) {
            LOGDBG("Download %p: finished\n", this);
            return false;
        }
    }

    return true;
}

bool
FileDownload::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case TaskClose:
        LOGDBG("Download %p: canceled (code %td)\n", this, item.value);
        return false;
    case TaskInput:
        handleData((std::string *)item.value);
        break;
    case TaskPause:
        m_throttled = true;
        enablefd(false);
        LOGDBG("Download %p: throttled (remote)\n", this);
        break;
    case TaskResume:
        m_throttled = false;
        enablefd(m_running);
        LOGDBG("Download %p: resumed\n", this);
        break;
    default:
        break;
    }

    return true;
}

bool
FileDownload::handleIdle()
{
    LOGNOT("Download %p: idle timeout exceeded\n", this);
    reportError(Tsq::TaskErrorTimedOut, strerror(ETIMEDOUT));
    return false;
}

bool
FileDownload::openfd()
{
    size_t total;
    uint32_t mode;

    try {
        setfd(osOpenFile(m_targetName.c_str(), &total, &mode));
    } catch (const std::exception &e) {
        // Fail
        reportError(Tsq::DownloadTaskErrorOpenFailed, strerror(errno));
        LOGDBG("Download %p: failed to open '%s' for reading: %m\n", this, m_targetName.c_str());
        return false;
    }

    // Send starting information
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addBytes(m_buf + 8, 48);
    m.addNumberPair(Tsq::TaskStarting, mode);
    m.addNumber64(total);
    g_listener->forwardToClient(m_clientId, m.result());
    LOGDBG("Download %p: running (%o %zu)\n", this, mode, total);
    return true;
}

void
FileDownload::threadMain()
{
    enablefd(false);

    try {
        if (openfd())
            runDescriptorLoop();
    }
    catch (const std::exception &e) {
        LOGERR("Download %p: caught exception: %s\n", this, e.what());
    }

    closefd();
    g_listener->sendWork(ListenerRemoveTask, this);
}

//
// Download from pipe
//
PipeDownload::PipeDownload(Tsq::ProtocolUnmarshaler *unm) :
    FileDownload(unm, 0)
{
    m_mode = unm->parseNumber() & 0777;
}

PipeDownload::~PipeDownload()
{
    unlink(m_targetName.c_str());
}

bool
PipeDownload::openfd()
{
    try {
        setfd(osCreateNamedPipe(true, m_mode, m_targetName));
    } catch (const std::exception &e) {
        // Fail
        reportError(Tsq::DownloadTaskErrorOpenFailed, e.what());
        LOGNOT("PipeIn %p: failed to create fifo: %s\n", this, e.what());
        return false;
    }

    // Send starting information
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addBytes(m_buf + 8, 48);
    m.addNumber(Tsq::TaskStarting);
    m.addString(m_targetName);
    g_listener->forwardToClient(m_clientId, m.result());
    LOGDBG("PipeIn %p: running\n", this);
    return true;
}

bool
PipeDownload::handleIdle()
{
    return true;
}
