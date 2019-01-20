// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "attributebase.h"
#include "basewatch.h"
#include "exception.h"

#include <pthread.h>

AttributeBase::AttributeBase(const char *name) :
    ThreadBase(name, ThreadBaseFd)
{
    if (pthread_rwlock_init(&m_rwlock, NULL) < 0)
        throw ErrnoException("pthread_rwlock_init", errno);
}

AttributeBase::~AttributeBase()
{
    forDeleteAll(m_watches);
    pthread_rwlock_destroy(&m_rwlock);
}

std::string
AttributeBase::lockedFindAttribute(const std::string &key) const
{
    // StateLock assumed to be held
    std::string spec;

    auto i = m_attributes.find(key);
    if (i != m_attributes.end())
        spec = i->second;

    return spec;
}

std::string
AttributeBase::lockedGetAttributes() const
{
    // StateLock assumed to be held
    std::string spec;

    for (const auto &i: m_attributes)
        if (i.first.front() != '_') {
            spec.append(i.first);
            spec.push_back('\0');
            spec.append(i.second);
            spec.push_back('\0');
        }

    return spec;
}

std::string
AttributeBase::commandGetAttributes() const
{
    StateLock slock(this, false);
    return lockedGetAttributes();
}

std::string
AttributeBase::commandGetAttribute(const std::string &key) const
{
    std::string spec = key;
    spec.push_back('\0');

    StateLock slock(this, false);

    auto i = m_attributes.find(key);
    if (i != m_attributes.end()) {
        spec.append(i->second);
        spec.push_back('\0');
    }

    return spec;
}

void
AttributeBase::commandSetAttribute(const std::string &key, const std::string &value)
{
    bool changed = false;

    {
        StateLock slock(this, true);

        auto i = m_attributes.find(key);
        if (i == m_attributes.end()) {
            m_attributes[key] = value;
            changed = true;
        } else if (i->second != value) {
            i->second = value;
            changed = true;
        }

        if (changed) {
            // Special handling for certain attributes set by clients
            reportAttributeChange(key, value);
        }
    }

    if (changed) {
        std::string spec = key;
        spec.push_back('\0');
        spec.append(value);
        spec.push_back('\0');

        Lock lock(this);

        for (auto watch: m_watches) {
            // two locks held
            watch->pushAttributeChange(key, spec);
        }
    }
}

void
AttributeBase::commandSetAttributes(StringMap &map)
{
    {
        StateLock slock(this, true);

        for (auto i = map.begin(); i != map.end(); )
        {
            auto j = m_attributes.find(i->first);
            if (j == m_attributes.end()) {
                m_attributes[i->first] = i->second;
                ++i;
            } else if (j->second != i->second) {
                j->second = i->second;
                ++i;
            } else {
                i = map.erase(i);
            }
        }
    }

    if (!map.empty()) {
        for (auto &&i: map) {
            std::string spec = i.first;
            spec.push_back('\0');
            spec.append(i.second);
            spec.push_back('\0');

            i.second = std::move(spec);
        }

        Lock lock(this);

        for (auto watch: m_watches) {
            // two locks held
            watch->pushAttributeChanges(map);
        }
    }
}

void
AttributeBase::commandRemoveAttribute(const std::string &key)
{
    StringMap::node_type nh;

    {
        StateLock slock(this, true);
        nh = m_attributes.extract(key);
    }

    if (nh) {
        auto &spec = nh.key();
        spec.push_back('\0');

        Lock lock(this);

        for (auto watch: m_watches) {
            // two locks held
            watch->pushAttributeChange(key, spec);
        }
    }
}

bool
AttributeBase::testAttribute(const std::string &key) const
{
    StateLock slock(this, false);

    auto i = m_attributes.find(key);
    return i != m_attributes.end() && i->second == "1"s;
}

void
AttributeBase::reportAttributeChange(const std::string &, const std::string &)
{
}
