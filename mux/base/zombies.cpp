// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "zombies.h"
#include "exception.h"
#include "os/wait.h"
#include "os/signal.h"
#include "os/logging.h"

#include <pthread.h>
#include <unistd.h>

TermReaper *g_reaper;

#define MAX_ORPHANS 128

TermReaper::TermReaper(const std::vector<int> &ignorepids) :
    ThreadBase("reaper", ThreadBaseCond),
    m_timeToDie(false)
{
    for (int pid: ignorepids)
        m_registered.emplace(pid, nullptr);
}

/*
 * Other threads
 */
void
TermReaper::stop(int)
{
    Lock lock(this);

    m_timeToDie = true;
    pthread_cond_signal(&m_cond);
}

void
TermReaper::registerProcess(ThreadBase *owner, int pid)
{
    bool immediate = false;
    int status;

    {
        Lock lock(this);

        auto i = m_orphans.find(pid);
        if (i != m_orphans.end()) {
            status = i->second;
            m_orphans.erase(i);
            immediate = true;
        } else {
            m_registered.emplace(pid, owner);
            pthread_cond_signal(&m_cond);
        }
    }

    if (immediate) {
        LOGDBG("Reaper: delivering pid %d (already received), status %d\n", pid, status);
        owner->sendWork(ReaperProcessExited, status);
    }
}

void
TermReaper::ignoreProcess(int pid)
{
    {
        Lock lock(this);

        auto i = m_orphans.find(pid);
        if (i != m_orphans.end()) {
            m_orphans.erase(i);
        } else {
            m_registered.emplace(pid, nullptr);
            pthread_cond_signal(&m_cond);
        }
    }
}

void
TermReaper::abandonProcess(int pid)
{
    Lock lock(this);

    auto i = m_registered.find(pid);
    if (i != m_registered.end())
        i->second = nullptr;
}

/*
 * This thread
 */
void
TermReaper::lockLoop()
{
    int pid, status;
    ThreadBase *owner;
    bool empty;

    while (1) {
        pid = -1;
        owner = nullptr;

        {
            Lock lock(this);

            empty = m_registered.empty();
            if (!m_timeToDie && empty) {
                pthread_cond_wait(&m_cond, &m_lock);
                empty = m_registered.empty();
            }
        }

        if (m_timeToDie || s_deathSignal)
            break;
        if (!empty)
            pid = osWaitForAny(&status);
        if (m_timeToDie || s_deathSignal)
            break;
        if (pid < 0)
            continue;

        {
            Lock lock(this);

            auto i = m_registered.find(pid);
            if (i != m_registered.end()) {
                owner = i->second;
                m_registered.erase(i);
            } else if (m_orphans.size() < MAX_ORPHANS) {
                m_orphans.emplace(pid, status);
            }
        }

        if (owner) {
            LOGDBG("Reaper: delivering pid %d, status %d\n", pid, status);
            owner->sendWork(ReaperProcessExited, status);
        }
    }
}

void
TermReaper::threadMain()
{
    detach();

    try {
        lockLoop();
    }
    catch (const TsqException &e) {
        LOGERR("Reaper: %s\n", e.what());
    } catch (const std::exception &e) {
        LOGERR("Reaper: caught exception: %s\n", e.what());
    }

    if (s_deathSignal)
        LOGDBG("Reaper: exiting on signal %d\n", s_deathSignal);
    else
        LOGDBG("Reaper: exiting\n");
}
