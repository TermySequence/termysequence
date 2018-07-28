// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "attributemap.h"
#include "lib/uuid.h"

#include <pthread.h>

class ThreadBase;
class TermReader;
class TermWriter;

enum WatchType {
    // Note: this ordering is used for watch sorting
    WatchTermProxy,
    WatchConnProxy,
    WatchServer,
    WatchTerm,
    WatchConn,
    WatchListener,
};

class BaseWatch
{
private:
    mutable pthread_mutex_t m_lock;
    unsigned m_refcount = 2;
    int m_release;

protected:
    ThreadBase *m_parent;
    TermReader *m_reader;
    TermWriter *m_writer;

    BaseWatch(ThreadBase *parent, TermReader *reader, WatchType type,
              int release, unsigned hops);

    // locked
    virtual void pushAnnounce() = 0;

public:
    class Lock;
    class FlexLock;
    class ActivatorLock;

    // unlocked
    const WatchType type;
    const unsigned hops;
    const unsigned serial;
    const bool isTermWatch;

    // locked
    bool active = false;
    bool closing = false;
    bool started = false;
    StringMap attributes;
    unsigned closeReason = 0;

public:
    virtual ~BaseWatch();

    virtual const Tsq::Uuid& parentId() const = 0;

    // unlocked
    void start();
    virtual void teardown();

    void requestRelease(unsigned reason);
    void release();
    void putReaderReference();
    void putWriterReference(FlexLock &wlock);

    // locked
    void pushAttributeChange(const std::string &key, const std::string &spec);
    void pushAttributeChanges(const StringMap &map);
    inline void activate();

public:
    // RAII locking interface
    class Lock {
        const BaseWatch *m_w;
    public:
        Lock(const BaseWatch *w);
        ~Lock();
    };

    class FlexLock {
        const BaseWatch *m_w;
    public:
        FlexLock(const BaseWatch *w);
        void unlock();
        ~FlexLock();
    };

    class ActivatorLock {
        BaseWatch *m_w;
    public:
        ActivatorLock(BaseWatch *w);
        ~ActivatorLock();
    };
};

struct WatchSorter
{
    bool operator()(const BaseWatch *lhs, const BaseWatch *rhs) const;
};

inline BaseWatch::Lock::Lock(const BaseWatch *w): m_w(w)
{
    pthread_mutex_lock(&m_w->m_lock);
}

inline BaseWatch::Lock::~Lock()
{
    pthread_mutex_unlock(&m_w->m_lock);
}

inline BaseWatch::FlexLock::FlexLock(const BaseWatch *w): m_w(w)
{
    pthread_mutex_lock(&m_w->m_lock);
}

inline void BaseWatch::FlexLock::unlock()
{
    pthread_mutex_unlock(&m_w->m_lock);
    m_w = nullptr;
}

inline BaseWatch::FlexLock::~FlexLock()
{
    if (m_w)
        pthread_mutex_unlock(&m_w->m_lock);
}

inline BaseWatch::ActivatorLock::ActivatorLock(BaseWatch *w): m_w(w)
{
    pthread_mutex_lock(&m_w->m_lock);
}

// BaseWatch::activate is in writer.h
// ActivatorLock::~ActivatorLock is in writer.h
