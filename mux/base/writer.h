// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "threadbase.h"
#include "eventstate.h"
#include "basewatch.h"

#include <unordered_set>
#include <set>
#include <queue>

class TermWatch;

class TermWriter final: public ThreadBase
{
    friend class TermReader;

private:
    TermReader *m_parent;
    TermEventTransfer m_transfer;

    std::unordered_set<BaseWatch*> m_watches;
    std::set<BaseWatch*,WatchSorter> *p_active, m_active, c_active;
    std::set<BaseWatch*,WatchSorter> m_closing, c_closing;
    std::queue<std::string> m_responses, c_responses;

    size_t m_bufferedAmount = 0;

    bool m_todo = false;
    bool m_stopping = false;
    bool m_throttled = false;
    bool m_started = false;

    void lockLoop();
    void threadMain();

    void handleProxyWatch(TermProxyWatch *watch);
    void handleTermWatch(TermWatch *watch);
    void handleBaseWatch(BaseWatch *watch);
    void handleClosingWatch(BaseWatch *watch);

public:
    TermWriter(TermReader *parent);
    ~TermWriter();
    void stop(int reason);

    void unblock();
    void setWatches(const std::set<BaseWatch*,WatchSorter> &watches);
    void addWatch(BaseWatch *watch);
    void activate(BaseWatch *watch);
    void requestRelease(BaseWatch *watch);

    bool submitResponse(std::string &&buf);

    void writeFd(int fd, const char *buf, size_t len);
};

inline void BaseWatch::activate()
{
    if (active)
        m_writer->activate(this);
}

inline BaseWatch::ActivatorLock::~ActivatorLock()
{
    m_w->activate();
    pthread_mutex_unlock(&m_w->m_lock);
}
