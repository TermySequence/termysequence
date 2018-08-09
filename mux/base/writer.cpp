// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "writer.h"
#include "reader.h"
#include "termwatch.h"
#include "proxywatch.h"
#include "listener.h"
#include "exception.h"
#include "os/conn.h"
#include "os/logging.h"
#include "lib/machine.h"
#include "lib/protocol.h"
#include "config.h"

#include <pthread.h>
#include <unistd.h>

TermWriter::TermWriter(TermReader *parent) :
    ThreadBase("writer", ThreadBaseCond),
    m_parent(parent)
{
    // Temporary location until writer is unblocked
    p_active = new std::set<BaseWatch*,WatchSorter>;
}

TermWriter::~TermWriter()
{
    if (p_active != &m_active)
        delete p_active;
}

void
TermWriter::setWatches(const std::set<BaseWatch*,WatchSorter> &watches)
{
    m_watches.insert(watches.begin(), watches.end());
}

/*
 * Other threads
 */
void
TermWriter::stop(int)
{
    Lock lock(this);

    m_stopping = true;
    pthread_cond_signal(&m_cond);
}

void
TermWriter::unblock()
{
    Lock lock(this);

    if (!m_started) {
        m_active = std::move(*p_active);
        delete p_active;
        p_active = &m_active;

        m_started = true;
        m_todo = true;
        pthread_cond_signal(&m_cond);
    }
}

void
TermWriter::addWatch(BaseWatch *watch)
{
    Lock lock(this);

    m_watches.emplace(watch);
}

void
TermWriter::activate(BaseWatch *watch)
{
    Lock lock(this);

    p_active->insert(watch);
    m_todo = true;
    pthread_cond_signal(&m_cond);
}

void
TermWriter::requestRelease(BaseWatch *watch)
{
    Lock lock(this);

    m_closing.insert(watch);
    m_todo = true;
    pthread_cond_signal(&m_cond);
}

bool
TermWriter::submitResponse(std::string &&buf)
{
    bool retval = true;

    Lock lock(this);

    if (!m_stopping) {
        m_bufferedAmount += buf.size();
        m_responses.push(std::move(buf));
        if (m_bufferedAmount > BUFFER_WARN_THRESHOLD) {
            m_throttled = true;
            retval = false;
        }
        m_todo = true;
        pthread_cond_signal(&m_cond);
    }

    return retval;
}

/*
 * This thread
 */
inline void
TermWriter::handleProxyWatch(TermProxyWatch *watch)
{
    {
        BaseWatch::Lock wlock(watch);
        m_transfer.transferBaseState(watch);
        // two locks held
        m_transfer.transferProxyState(watch);
    }

    m_transfer.writeProxyResponses(m_parent);
    m_transfer.writeBaseResponses(m_parent);
}

inline void
TermWriter::handleTermWatch(TermWatch *watch)
{
    {
        BaseWatch::Lock wlock(watch);
        m_transfer.transferBaseState(watch);
        // two locks held
        m_transfer.transferTermState(watch);
    }

    m_transfer.writeTermResponses(m_parent);
    m_transfer.writeBaseResponses(m_parent);
}

inline void
TermWriter::handleBaseWatch(BaseWatch *watch)
{
    {
        BaseWatch::Lock wlock(watch);
        m_transfer.transferBaseState(watch);
    }

    m_transfer.writeBaseResponses(m_parent);
}

void
TermWriter::handleClosingWatch(BaseWatch *watch)
{
    bool closable;
    unsigned reason;

    {
        BaseWatch::FlexLock wlock(watch);
        m_transfer.transferBaseState(watch);

        closable = watch->started;
        reason = watch->closeReason;
        {
            Lock lock(this);
            p_active->erase(watch);
            m_watches.erase(watch);
        }
        watch->putWriterReference(wlock);
    }

    if (closable)
        m_transfer.writeClosing(m_parent, reason);
}

void
TermWriter::lockLoop()
{
    bool wasThrottled;

    while (1) {
        {
            Lock lock(this);

            if (!m_stopping && !m_todo)
                pthread_cond_wait(&m_cond, &m_lock);
            if (m_stopping || s_deathSignal)
                break;

            m_responses.swap(c_responses);
            m_active.swap(c_active);
            m_closing.swap(c_closing);

            m_bufferedAmount = 0;
            m_todo = false;
            wasThrottled = m_throttled;
            m_throttled = false;
        }

        if (wasThrottled)
            TermReader::pushTaskResume(g_listener->id());

        while (!c_responses.empty()) {
            const std::string &buf = c_responses.front();
            m_parent->machine()->connSend(buf.data(), buf.size());
            c_responses.pop();
        }

        for (auto i = c_active.begin(); i != c_active.end(); i = c_active.erase(i))
            switch ((*i)->type) {
            case WatchTermProxy:
                handleProxyWatch(static_cast<TermProxyWatch*>(*i));
                break;
            case WatchTerm:
                handleTermWatch(static_cast<TermWatch*>(*i));
                break;
            default:
                handleBaseWatch(*i);
                break;
            }

        for (auto i = c_closing.begin(); i != c_closing.end(); i = c_closing.erase(i))
            handleClosingWatch(*i);

        m_parent->machine()->connFlush(nullptr, 0);
    }
}

void
TermWriter::threadMain()
{
    bool error = false;

    try {
        lockLoop();
    }
    catch (const TsqException &e) {
        LOGERR("Writer %p: %s\n", this, e.what());
        error = true;
    } catch (const std::exception &e) {
        LOGERR("Writer %p: caught exception: %s\n", this, e.what());
        error = true;
    }

    if (error) {
        {
            Lock lock(this);
            m_stopping = true;
        }
        if (!s_deathSignal)
            m_parent->stop(TSQ_STATUS_SERVER_ERROR);
    }
}

void
TermWriter::writeFd(int fd, const char *buf, size_t len)
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
        buf += rc;
        len -= rc;
    }
}
