// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "baseproxy.h"
#include "basewatch.h"
#include "conn.h"
#include "exception.h"
#include "lib/protocol.h"
#include "lib/wire.h"

#include <pthread.h>
#include <cassert>

BaseProxy::BaseProxy(ConnInstance *parent, int release) :
    m_parent(parent),
    m_release(release),
    m_closing(false)
{
    if (pthread_mutex_init(&m_lock, NULL) < 0)
        throw ErrnoException("pthread_mutex_init", errno);
    if (pthread_rwlock_init(&m_rwlock, NULL) < 0)
        throw ErrnoException("pthread_rwlock_init", errno);
}

BaseProxy::~BaseProxy()
{
    assert(m_watches.empty());
    pthread_rwlock_destroy(&m_rwlock);
    pthread_mutex_destroy(&m_lock);
}

std::string
BaseProxy::lockedGetAttributes() const
{
    std::string spec;

    for (const auto &i: m_attributes) {
        spec.append(i.first);
        spec.push_back('\0');
        spec.append(i.second);
        spec.push_back('\0');
    }

    return spec;
}

bool
BaseProxy::addWatch(BaseWatch *watch)
{
    Lock lock(this);

    if (m_closing) {
        return false;
    } else {
        m_watches.insert(watch);
        return true;
    }
}

void
BaseProxy::removeWatch(BaseWatch *watch)
{
    bool finished;

    {
        Lock lock(this);

        m_watches.erase(watch);
        finished = m_closing && m_watches.empty();
    }

    if (finished)
        m_parent->sendWork(m_release, this);
}

void
BaseProxy::requestRelease(unsigned reason)
{
    bool finished = false;

    {
        Lock lock(this);

        if (!m_closing) {
            m_closing = true;
            finished = m_watches.empty();

            // ask readers nicely to release their watches
            reason ^= TSQ_FLAG_PROXY_CLOSED;
            for (auto watch: m_watches)
                watch->requestRelease(reason);
        }
    }

    if (finished)
        m_parent->sendWork(m_release, this);
}

void
BaseProxy::wireAttribute(const char *body, uint32_t length)
{
    Tsq::ProtocolUnmarshaler unm(body, length);
    std::string key(unm.parseString());
    std::string spec = key;
    spec.push_back('\0');

    bool changed = false;

    {
        StateLock lock(this, true);

        if (unm.remainingLength()) {
            std::string value(unm.parseString());

            auto i = m_attributes.find(key);
            if (i == m_attributes.end() || i->second != value) {
                m_attributes[key] = value;

                spec.append(value);
                spec.push_back('\0');
                changed = true;
            }
        }
        else {
            changed = (m_attributes.erase(key) != 0);
        }
    }

    if (changed) {
        Lock lock(this);

        for (auto watch: m_watches) {
            // two locks held
            watch->pushAttributeChange(key, spec);
        }
    }
}
