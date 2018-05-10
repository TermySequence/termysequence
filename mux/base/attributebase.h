// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "threadbase.h"
#include "attributemap.h"
#include "lib/uuid.h"

#include <unordered_set>
#include <pthread.h>

class BaseWatch;

class AttributeBase: public ThreadBase
{
protected:
    mutable pthread_rwlock_t m_rwlock;

    std::unordered_set<BaseWatch*> m_watches;
    AttributeMap m_attributes;

    Tsq::Uuid m_id;

public:
    AttributeBase(const char *name);
    ~AttributeBase();

    inline const Tsq::Uuid& id() const { return m_id; }
    inline bool noWatches() const { return m_watches.empty(); }

    std::string lockedGetAttributes() const;
    std::string commandGetAttributes() const;
    std::string commandGetAttribute(const std::string &key) const;
    void commandSetAttribute(const std::string &key, const std::string &value);
    void commandSetAttributes(AttributeMap &map);
    void commandRemoveAttribute(const std::string &key);
    bool testAttribute(const std::string &key) const;

private:
    virtual void reportAttributeChange(const std::string &key, const std::string &value);

public:
    // RAII locking interface
    class StateLock {
        const AttributeBase *m_t;
    public:
        StateLock(const AttributeBase *t, bool write);
        ~StateLock();
    };
};

inline AttributeBase::StateLock::StateLock(const AttributeBase *t, bool write): m_t(t)
{
    if (write)
        pthread_rwlock_wrlock(&m_t->m_rwlock);
    else
        pthread_rwlock_rdlock(&m_t->m_rwlock);
}

inline AttributeBase::StateLock::~StateLock()
{
    pthread_rwlock_unlock(&m_t->m_rwlock);
}
