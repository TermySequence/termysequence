// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "os/signal.h"

#include <queue>
#include <vector>
#include <csignal>
#include <pthread.h>
#include <poll.h>

struct WorkItem {
    int type;
    int value2;
    intptr_t value;

    inline WorkItem() {}
    inline WorkItem(int t, intptr_t v, int v2 = 0) :
        type(t), value2(v2), value(v) {}
};

extern "C" void* threadTrampoline(void *arg);

class ThreadBase
{
    friend void* threadTrampoline(void *arg);
    friend void deathHandler(int signal);
    friend void reloadHandler(int signal);

private:
    int m_eventfd[2]{ -1, -1 };
    std::queue<WorkItem> m_workQueue;

    bool m_event = false;
    bool m_started = false;
    bool m_confirmed = false;
    bool m_interrupted = false;
    const bool m_usesCondition;

    pthread_t m_tid;
    const char *m_tname;

    bool nextWorkItem(WorkItem &result);

    virtual void threadMain() = 0;
    virtual bool handleFd();
    virtual bool handleInterrupt();
    virtual bool handleWork(const WorkItem &item);
    virtual bool handleIdle();
    virtual bool handleMultiFd(pollfd &pfd);

protected:
    mutable pthread_mutex_t m_lock;
    pthread_cond_t m_cond;

    static volatile sig_atomic_t s_deathSignal;
    static int s_reloadFd[2];
    int m_pid = 0;
    int m_exitStatus = 0;
    int m_fd = -1;
    int m_timeout = -1;
    std::vector<pollfd> m_fds;

    void runDescriptorLoop();
    void runDescriptorLoopWithoutFd();
    void runDescriptorLoopMulti();
    void closefd();

    inline void setfd(int fd) { m_fds[1].fd = m_fd = fd; }
    inline void loadfd() { m_fds[1].fd = m_fd; }
    inline void enablefd(bool on) { m_fds[1].events = on ? POLLIN : 0; }
    inline void togglefd() { m_fds[1].events = !m_fds[1].events ? POLLIN : 0; }

    void setKeepalive(int timeout, int multiplier);
    // Locked
    void commitWork();
    inline void stageWork(int type, void *ptr)
    { m_workQueue.emplace(type, (intptr_t)ptr); }

    enum ThreadBaseFlags {
        ThreadBaseMulti = 0, ThreadBaseFd = 1, ThreadBaseCond = 2,
        TaskBaseThrottlable = 4, TaskBaseExclusive = 8
    };

public:
    class Lock;
    class FlexLock;

    ThreadBase(const char *name, unsigned flags);
    virtual ~ThreadBase();

    inline bool started() const { return m_started; }
    inline bool confirmed() const { return m_confirmed; }
    inline bool interrupted() const { return m_interrupted; }
    inline int exitStatus() const { return m_exitStatus; }

    void start(int fd);
    int run(int fd);
    void detach();

    void sendWork(int type, int value1, int value2 = 0);
    void sendWork(int type, void *ptr);
    void sendWorkAndUnlock(FlexLock &lock, int type, void *ptr, int value2 = 0);

    inline void confirm() { m_confirmed = true; }
    inline void stop(int reason) { sendWork(0, reason); }
    void join();
    void closeevents();

public:
    // RAII locking interface
    class Lock {
        const ThreadBase *m_t;
    public:
        Lock(const ThreadBase *t);
        ~Lock();
    };

    class FlexLock {
        const ThreadBase *m_t;
    public:
        FlexLock(const ThreadBase *t);
        void unlock();
        ~FlexLock();
    };
};

inline ThreadBase::Lock::Lock(const ThreadBase *t): m_t(t)
{
    pthread_mutex_lock(&m_t->m_lock);
}

inline ThreadBase::Lock::~Lock()
{
    pthread_mutex_unlock(&m_t->m_lock);
}

inline ThreadBase::FlexLock::FlexLock(const ThreadBase *t): m_t(t)
{
    pthread_mutex_lock(&m_t->m_lock);
}

inline void ThreadBase::FlexLock::unlock()
{
    pthread_mutex_unlock(&m_t->m_lock);
    m_t = nullptr;
}

inline ThreadBase::FlexLock::~FlexLock()
{
    if (m_t)
        pthread_mutex_unlock(&m_t->m_lock);
}
