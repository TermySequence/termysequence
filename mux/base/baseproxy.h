// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "attributemap.h"
#include "lib/uuid.h"

#include <unordered_set>
#include <pthread.h>

class ConnInstance;
class BaseWatch;

class BaseProxy
{
protected:
    mutable pthread_mutex_t m_lock;
    mutable pthread_rwlock_t m_rwlock;

    ConnInstance *m_parent;

    Tsq::Uuid m_id;
    Tsq::Uuid m_hopId;

    std::unordered_set<BaseWatch*> m_watches;

    StringMap m_attributes;
    unsigned m_nHops;

    void wireAttribute(const char *body, uint32_t length);

private:
    int m_release;
    bool m_closing;

public:
    BaseProxy(ConnInstance *parent, int release);
    ~BaseProxy();

    inline ConnInstance* parent() { return m_parent; }
    inline const Tsq::Uuid& id() const { return m_id; }
    inline const Tsq::Uuid& hopId() const { return m_hopId; }
    inline unsigned nHops() const { return m_nHops; }

    std::string lockedGetAttributes() const;

    bool addWatch(BaseWatch *watch);
    void removeWatch(BaseWatch *watch);

    void requestRelease(unsigned reason);

public:
    // RAII locking interface
    class Lock {
        const BaseProxy *m_p;
    public:
        Lock(const BaseProxy *p);
        ~Lock();
    };

    class StateLock {
        const BaseProxy *m_p;
    public:
        StateLock(const BaseProxy *p, bool write);
        ~StateLock();
    };
};

inline BaseProxy::Lock::Lock(const BaseProxy *p): m_p(p)
{
    pthread_mutex_lock(&m_p->m_lock);
}

inline BaseProxy::Lock::~Lock()
{
    pthread_mutex_unlock(&m_p->m_lock);
}

inline BaseProxy::StateLock::StateLock(const BaseProxy *p, bool write): m_p(p)
{
    if (write)
        pthread_rwlock_wrlock(&m_p->m_rwlock);
    else
        pthread_rwlock_rdlock(&m_p->m_rwlock);
}

inline BaseProxy::StateLock::~StateLock()
{
    pthread_rwlock_unlock(&m_p->m_rwlock);
}
