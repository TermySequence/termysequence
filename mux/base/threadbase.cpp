// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "threadbase.h"
#include "exception.h"
#include "os/eventfd.h"
#include "os/process.h"
#include "config.h"

#include <pthread.h>
#include <unistd.h>
#include <cassert>

volatile sig_atomic_t ThreadBase::s_deathSignal;
int ThreadBase::s_reloadFd[2] = { -1, -1 };

struct ThreadArg {
    ThreadBase *obj;
    int fd;
};

static inline void
renameThread(const char *name)
{
    if (name) {
        char buf[16] = ABBREV_NAME "-";
        strncat(buf, name, sizeof(buf) - sizeof(ABBREV_NAME) - 1);
        osRenameThread(buf);
    }
}

extern "C" void* threadTrampoline(void *argPtr) {
    ThreadArg *arg = static_cast<ThreadArg*>(argPtr);
    ThreadBase *obj = arg->obj;

    obj->m_fd = arg->fd;
    delete arg;

    renameThread(obj->m_tname);

    obj->threadMain();
    return NULL;
}

void
ThreadBase::start(int fd)
{
    assert(!m_started);

    ThreadArg *arg = new ThreadArg;
    arg->obj = this;
    arg->fd = fd;

    int rc = pthread_create(&m_tid, NULL, &threadTrampoline, arg);
    if (rc < 0) {
        delete arg;
        throw ErrnoException("pthread_create", errno);
    }

    m_started = true;
}

int
ThreadBase::run(int fd)
{
    m_fd = fd;
    renameThread(m_tname);
    threadMain();
    return m_exitStatus;
}

ThreadBase::ThreadBase(const char *name, unsigned flags) :
    m_usesCondition(flags & ThreadBaseCond),
    m_tname(name)
{
    if (pthread_mutex_init(&m_lock, NULL) < 0)
        throw ErrnoException("pthread_mutex_init", errno);

    if (m_usesCondition) {
        if (pthread_cond_init(&m_cond, NULL) < 0)
            throw ErrnoException("pthread_cond_init", errno);
    } else {
        if (osInitEvent(m_eventfd) < 0)
            throw ErrnoException("eventfd", errno);

        m_fds.emplace_back(pollfd{ .fd = m_eventfd[0], .events = POLLIN });

        if (flags & ThreadBaseFd)
            m_fds.emplace_back(pollfd{ .fd = -1, .events = POLLIN });
    }
}

ThreadBase::~ThreadBase()
{
    assert(m_fd == -1);

    if (m_eventfd[0] != -1)
        osCloseEvent(m_eventfd);
    if (m_usesCondition)
        pthread_cond_destroy(&m_cond);

    pthread_mutex_destroy(&m_lock);
}

void
ThreadBase::sendWork(int type, int value1, int value2)
{
    {
        Lock lock(this);
        m_workQueue.emplace(type, value1, value2);

        if (m_event)
            return;

        m_event = true;
    }

    osSetEvent(m_eventfd[1]);
}

void
ThreadBase::sendWork(int type, void *ptr)
{
    {
        Lock lock(this);
        m_workQueue.emplace(type, (intptr_t)ptr);

        if (m_event)
            return;

        m_event = true;
    }

    osSetEvent(m_eventfd[1]);
}

void
ThreadBase::commitWork()
{
    if (!m_event) {
        m_event = true;
        osSetEvent(m_eventfd[1]);
    }
}

void
ThreadBase::sendWorkAndUnlock(FlexLock &lock, int type, void *ptr, int value2)
{
    m_workQueue.emplace(type, (intptr_t)ptr, value2);

    if (m_event) {
        lock.unlock();
    } else {
        m_event = true;
        lock.unlock();
        osSetEvent(m_eventfd[1]);
    }
}

bool
ThreadBase::handleFd()
{
    return false;
}

bool
ThreadBase::handleMultiFd(pollfd &)
{
    return false;
}

bool
ThreadBase::handleInterrupt()
{
    return false;
}

bool
ThreadBase::handleWork(const WorkItem &item)
{
    m_exitStatus = item.value;
    return false;
}

bool
ThreadBase::handleIdle()
{
    return true;
}

inline bool
ThreadBase::nextWorkItem(WorkItem &result)
{
    osClearEvent(m_eventfd[0]);

    Lock lock(this);

    if (!m_workQueue.empty()) {
        result = m_workQueue.front();
        m_workQueue.pop();
        return true;
    }

    return m_event = false;
}

void
ThreadBase::runDescriptorLoop()
{
    WorkItem item;
    pollfd *pfds = m_fds.data();
    pfds[1].fd = m_fd;

    while (true) {
        switch (poll(pfds, 2, m_timeout)) {
        default:
            if (pfds[1].revents && !handleFd())
                return;
            if (pfds[0].revents)
                while (nextWorkItem(item))
                    if (!handleWork(item))
                        return;
            break;
        case 0:
            if (!handleIdle())
                return;
            break;
        case -1:
            if (errno != EINTR && errno != EAGAIN)
                throw Tsq::ErrnoException("poll", errno);
            break;
        }
    }
}

void
ThreadBase::runDescriptorLoopWithoutFd()
{
    WorkItem item;
    pollfd *pfd = m_fds.data();

    while (true) {
        switch (poll(pfd, 1, m_timeout)) {
        default:
            while (nextWorkItem(item))
                if (!handleWork(item))
                    return;
            break;
        case 0:
            if (!handleIdle())
                return;
            break;
        case -1:
            if (errno != EINTR && errno != EAGAIN)
                throw Tsq::ErrnoException("poll", errno);
            break;
        }
    }
}

void
ThreadBase::runDescriptorLoopMulti()
{
    WorkItem item;

    if (s_deathSignal) {
        m_interrupted = true;
        if (!handleInterrupt())
            return;
    }

    while (true) {
        if (poll(m_fds.data(), m_fds.size(), -1) < 0)
            if (errno != EINTR && errno != EAGAIN)
                throw Tsq::ErrnoException("poll", errno);

        if (s_deathSignal && !m_interrupted) {
            m_interrupted = true;
            if (handleInterrupt())
                continue;
            else
                return;
        }

        if (m_fds[0].revents & POLLIN)
            while (nextWorkItem(item))
                if (!handleWork(item))
                    return;

        for (unsigned i = m_fds.size() - 1; i >= 1; --i)
            if (m_fds[i].revents && !handleMultiFd(m_fds[i]))
                return;
    }
}

void
ThreadBase::closefd()
{
    if (m_fd != -1) {
        close(m_fd);
        setfd(-1);
    }
}

void
ThreadBase::closeevents()
{
    osCloseEvent(m_eventfd);
    m_eventfd[0] = m_eventfd[1] = -1;
    m_fds[0].fd = -1;
}

void
ThreadBase::detach()
{
    pthread_detach(m_tid);
}

void
ThreadBase::join()
{
    assert(m_started);
    pthread_join(m_tid, NULL);
    m_started = false;
}

void
ThreadBase::setKeepalive(int timeout, int multiplier)
{
    if (timeout > 0) {
        m_timeout = (timeout > KEEPALIVE_MIN) ? timeout : KEEPALIVE_MIN;
        m_timeout *= multiplier;
    }
}
