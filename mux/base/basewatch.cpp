// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "basewatch.h"
#include "threadbase.h"
#include "reader.h"
#include "writer.h"
#include "exception.h"
#include "os/logging.h"

#include <pthread.h>
#include <errno.h>

// Note: all watches are created from the listener thread
static unsigned s_nextSerial;

#ifndef NDEBUG
static const char *
watchName(WatchType type)
{
    switch (type) {
    case WatchServer:
        return "ServerWatch";
    case WatchListener:
        return "ListenerWatch";
    case WatchConn:
        return "ConnWatch";
    case WatchTerm:
        return "TermWatch";
    default:
        return "ProxyWatch";
    }
}
#endif

BaseWatch::BaseWatch(ThreadBase *parent, TermReader *reader, WatchType type_,
                     int release, unsigned hops_) :
    m_release(release),
    m_parent(parent),
    m_reader(reader),
    m_writer(reader->writer()),
    type(type_),
    hops(hops_),
    serial(s_nextSerial++),
    isTermWatch(type == WatchTerm)
{
    int rc = pthread_mutex_init(&m_lock, NULL);
    if (rc < 0)
        throw ErrnoException("pthread_mutex_init", errno);

    LOGDBG("%s %p: created (Parent %p -> Reader %p, %u hops)\n",
           watchName(type), this, parent, reader, hops);
}

BaseWatch::~BaseWatch()
{
    pthread_mutex_destroy(&m_lock);
    LOGDBG("%s %p: destroyed\n", watchName(type), this);
}

void
BaseWatch::start()
{
    Lock wlock(this);

    if (!closing) {
        pushAnnounce();
        active = true;
        started = true;
        m_writer->activate(this);
    }
}

void
BaseWatch::pushAttributeChange(const std::string &key, const std::string &spec)
{
    Lock wlock(this);

    if (active) {
        attributes[key] = spec;
        m_writer->activate(this);
    }
}

void
BaseWatch::pushAttributeChanges(const StringMap &map)
{
    Lock wlock(this);

    if (active) {
        for (const auto &i: map) {
            attributes[i.first] = i.second;
        }
        m_writer->activate(this);
    }
}

/*
 * Called from parent
 */
void
BaseWatch::requestRelease(unsigned reason)
{
    Lock wlock(this);

    active = false;
    closing = true;
    closeReason = reason;

    if (m_reader) {
        // two locks held
        m_reader->sendWork(ReaderReleaseWatch, this);
    }
    if (m_writer) {
        // two locks held
        m_writer->requestRelease(this);
    }
}

/*
 * Called from reader's destructor
 */
void
BaseWatch::release()
{
    {
        Lock wlock(this);
        active = false;
        closing = true;
        m_reader = nullptr;
        m_writer = nullptr;
    }

    m_parent->sendWork(m_release, this);
}

/*
 * Called from reader and writer
 */
void
BaseWatch::putReaderReference()
{
    bool done;

    {
        Lock wlock(this);
        active = false;
        m_reader = nullptr;
        done = (--m_refcount == 0);
    }

    if (done)
        m_parent->sendWork(m_release, this);
}

void
BaseWatch::putWriterReference(FlexLock &wlock)
{
    active = false;
    m_writer = nullptr;
    bool done = (--m_refcount == 0);

    wlock.unlock();

    if (done)
        m_parent->sendWork(m_release, this);
}

void
BaseWatch::teardown()
{
}

bool
WatchSorter::operator()(const BaseWatch *lhs, const BaseWatch *rhs) const
{
    if (lhs->hops != rhs->hops)
        return lhs->hops > rhs->hops;
    if (lhs->type != rhs->type)
        return lhs->type < rhs->type;
    if (lhs->serial != rhs->serial)
        return lhs->serial < rhs->serial;

    return lhs < rhs;
}
