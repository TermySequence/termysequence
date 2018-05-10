// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "imagetask.h"
#include "listener.h"
#include "term.h"
#include "emulator.h"
#include "exception.h"
#include "os/logging.h"
#include "lib/wire.h"
#include "lib/protocol.h"

#define HEADERSIZE 60

ImageDownload::ImageDownload(Tsq::ProtocolUnmarshaler *unm, TermInstance *term) :
    TaskBase("getimage", unm, TaskBaseThrottlable),
    m_sent(0),
    m_acked(0),
    m_savedTimeout(m_timeout),
    m_running(false)
{
    contentid_t id = unm->parseNumber64();
    m_data = term->emulator()->getContent(id);
    m_targetName = std::to_string(id);
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

ImageDownload::~ImageDownload()
{
    delete [] m_buf;
}

/*
 * This thread
 */
void
ImageDownload::handleData(std::string *data)
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
        if (!m_throttled)
            m_timeout = 0;
    }
}

void
ImageDownload::pushBytes(size_t len)
{
    uint32_t length = htole32(HEADERSIZE - 8 + len);
    memcpy(m_buf + 4, &length, 4);

    std::string buf(m_buf, HEADERSIZE + len);
    m_sent += len;

    if (!throttledOutput(buf)) {
        LOGDBG("GetImage %p: throttled (local)\n", this);
        m_throttled = true;
        m_timeout = m_savedTimeout;
    }
}

bool
ImageDownload::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case TaskClose:
        LOGDBG("GetImage %p: canceled (code %td)\n", this, item.value);
        return false;
    case TaskInput:
        handleData((std::string *)item.value);
        break;
    case TaskPause:
        m_throttled = true;
        m_timeout = m_savedTimeout;
        LOGDBG("GetImage %p: throttled (remote)\n", this);
        break;
    case TaskResume:
        m_throttled = false;
        if (m_running)
            m_timeout = 0;
        LOGDBG("GetImage %p: resumed\n", this);
        break;
    default:
        break;
    }

    return true;
}

bool
ImageDownload::handleIdle()
{
    if (m_timeout) {
        LOGNOT("GetImage %p: idle timeout exceeded\n", this);
        reportError(Tsq::TaskErrorTimedOut, strerror(ETIMEDOUT));
        return false;
    }

    if (m_sent - m_acked >= m_windowSize * m_chunkSize) {
        // Stop and wait for ack message
        m_running = false;
        m_timeout = m_savedTimeout;
    }
    else {
        size_t rem = m_data->size() - m_sent;
        size_t rc = (rem > m_chunkSize) ? m_chunkSize : rem;
        memcpy(m_ptr, m_data->data() + m_sent, rc);

        if (rc == 0) {
            pushBytes(0);
            LOGDBG("GetImage %p: finished\n", this);
            return false;
        }
        else {
            pushBytes(rc);
            // LOGDBG("GetImage %p: wrote %zu bytes\n", this, rc);
        }
    }

    return true;
}

bool
ImageDownload::begin()
{
    if (!m_data) {
        // Fail
        reportError(Tsq::DownloadTaskErrorNoSuchImage, strerror(ENOENT));
        LOGDBG("GetImage %p: content '%s' not found\n", this, m_targetName.c_str());
        return false;
    }

    // Send starting information
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addBytes(m_buf + 8, 48);
    m.addNumberPair(Tsq::TaskStarting, 0644);
    m.addNumber64(m_data->size());
    g_listener->forwardToClient(m_clientId, m.result());
    LOGDBG("GetImage %p: running (%zu)\n", this, m_data->size());
    return true;
}

void
ImageDownload::threadMain()
{
    try {
        if (begin())
            runDescriptorLoopWithoutFd();
    }
    catch (const std::exception &e) {
        LOGERR("GetImage %p: caught exception: %s\n", this, e.what());
    }

    g_listener->sendWork(ListenerRemoveTask, this);
}
