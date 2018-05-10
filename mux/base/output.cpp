// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "output.h"
#include "conn.h"
#include "exception.h"
#include "os/conn.h"
#include "os/logging.h"
#include "config.h"

#include <pthread.h>
#include <unistd.h>

TermOutput::TermOutput(ConnInstance *parent) :
    ThreadBase("output", ThreadBaseCond),
    m_parent(parent),
    m_bufferedAmount(0),
    m_throttled(false),
    m_stopping(false)
{
}

/*
 * Other threads
 */
void
TermOutput::stop(int)
{
    Lock lock(this);

    m_stopping = true;
    pthread_cond_signal(&m_cond);
}

void
TermOutput::reset()
{
    // Must not be called when this thread is running
    m_data = std::queue<std::string>();
    c_data = std::queue<std::string>();
    m_commands = std::queue<std::string>();
    c_commands = std::queue<std::string>();
    m_bufferedAmount = 0;
    m_throttled = false;
    m_stopping = false;
}

bool
TermOutput::submitData(std::string &&buf)
{
    bool retval = true;

    Lock lock(this);

    if (!m_stopping) {
        m_bufferedAmount += buf.size();
        m_data.push(buf);
        if (m_bufferedAmount > BUFFER_WARN_THRESHOLD) {
            m_throttled = true;
            retval = false;
        }
        pthread_cond_signal(&m_cond);
    }

    return retval;
}

bool
TermOutput::submitCommand(std::string &&buf)
{
    bool retval = true;

    Lock lock(this);

    if (!m_stopping) {
        m_bufferedAmount += buf.size();
        m_commands.push(buf);
        if (m_bufferedAmount > BUFFER_WARN_THRESHOLD) {
            m_throttled = true;
            retval = false;
        }
        pthread_cond_signal(&m_cond);
    }

    return retval;
}

size_t
TermOutput::bufferCurrentAmount() const
{
    Lock lock(this);
    return m_bufferedAmount;
}

size_t
TermOutput::bufferWarnAmount() const
{
    return BUFFER_WARN_THRESHOLD;
}

/*
 * This thread
 */
void
TermOutput::lockLoop()
{
    bool wasThrottled;

    while (1) {
        {
            Lock lock(this);

            if (!m_stopping && m_data.empty() && m_commands.empty())
                pthread_cond_wait(&m_cond, &m_lock);
            if (m_stopping || s_deathSignal)
                break;

            m_data.swap(c_data);
            m_commands.swap(c_commands);

            m_bufferedAmount = 0;
            wasThrottled = m_throttled;
            m_throttled = false;
        }

        if (wasThrottled)
            ConnInstance::pushThrottleResume(m_parent->id());

        if (!c_data.empty()) {
            do {
                const std::string &buf = c_data.front();
                m_parent->writeFd(buf.data(), buf.size());
                c_data.pop();
            } while (!c_data.empty());
            m_parent->sendWork(TermInputSent, 0);
        }
        if (!c_commands.empty()) {
            do {
                const std::string &buf = c_commands.front();
                m_parent->machine()->connSend(buf.data(), buf.size());
                c_commands.pop();
            } while (!c_commands.empty());
            m_parent->machine()->connFlush(nullptr, 0);
        }
    }
}

void
TermOutput::threadMain()
{
    bool error = false;

    try {
        lockLoop();
    }
    catch (const TsqException &e) {
        LOGERR("Output %p: %s\n", this, e.what());
        error = true;
    } catch (const std::exception &e) {
        LOGERR("Output %p: caught exception: %s\n", this, e.what());
        error = true;
    }

    if (error) {
        Lock lock(this);
        m_stopping = true;
    }
}

void
TermOutput::writeFd(int fd, const char *buf, size_t len)
{
    while (len) {
        ssize_t rc = write(fd, buf, len);
        if (rc < 0) {
            int code = errno;

            {
                Lock lock(this);
                if (m_stopping || s_deathSignal)
                    throw ErrnoException(EINTR);
            }

            if (code == EAGAIN || code == EINTR) {
                osWaitForWritable(fd);
                continue;
            }

            throw ErrnoException("write", code);
        }
        len -= rc;
        buf += rc;
    }
}
