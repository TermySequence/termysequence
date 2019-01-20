// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#if USE_SYSTEMD

#include "base/threadbase.h"
#include "lib/uuid.h"
#include "os/conn.h"

#include <unordered_map>
#include <map>
#include <systemd/sd-bus.h>

class TermInstance;

extern "C" int matchJobRemoved(sd_bus_message*, void*, sd_bus_error*);

class TermScoper final: public ThreadBase
{
    friend int matchJobRemoved(sd_bus_message*, void*, sd_bus_error*);

private:
    struct ScopeInfo {
        TermInstance *term;
        Tsq::Uuid termId;
        int startingPid;
        int waitFd;
        std::string unitName;
    };

    sd_bus *m_bus = nullptr;

    // locked
    std::unordered_map<TermInstance*,ScopeInfo*> m_terms;
    // unlocked
    std::map<std::string,ScopeInfo*> m_units;

private:
    void openBus();
    void closeBus();

    void threadMain();
    bool handleFd();
    void handleJob(const char *unitName, const char *result);

    bool handleWork(const WorkItem &item);
    void handleCreate(ScopeInfo *info);
    void handleAbandon(ScopeInfo *info);

public:
    TermScoper();

    void createScope(TermInstance *term, int fd, int pid, bool restart);
    void unregisterTerm(TermInstance *term);
};

enum ScoperWork {
    ScoperClose,
    ScoperCreate,
    ScoperAbandon,
};

extern TermScoper *g_scoper;

//
// Threadbase macros
//
#define sd_createScoper() \
    g_scoper = new TermScoper()
#define sd_startScoper(fd) \
    g_scoper->start(fd)
#define sd_stopScoper(reason) \
    g_scoper->stop(reason); \
    g_scoper->join()

//
// TermInstance macros
//
#define sd_prepareScope() \
    if (osPipe(m_params->waitFd) != 0) \
        throw Tsq::ErrnoException(errno); \
    m_params->waitForFd = true

#define sd_createScope() \
    g_scoper->createScope(this, m_params->waitFd[1], m_pid, m_haveRestarted)

#define sd_cleanupScope(i) \
    close(m_params->waitFd[i])

#define sd_unregisterTerm() \
    g_scoper->unregisterTerm(this)

#else // USE_SYSTEMD

#define sd_createScoper()
#define sd_startScoper(fd)
#define sd_stopScoper(reason)

#define sd_prepareScope()
#define sd_createScope(term, pid)
#define sd_cleanupScope(i)
#define sd_unregisterTerm(term)

#endif // USE_SYSTEMD
