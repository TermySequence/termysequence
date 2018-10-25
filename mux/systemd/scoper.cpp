// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "scoper.h"
#include "base/exception.h"
#include "base/term.h"
#include "os/logging.h"
#include "lib/attrstr.h"

#include <unistd.h>

TermScoper *g_scoper;

TermScoper::TermScoper() :
    ThreadBase("scopemgr", ThreadBaseFd)
{}

void
TermScoper::closeBus()
{
    if (m_bus) {
        sd_bus_flush(m_bus);
        sd_bus_close(m_bus); // closes fd
        sd_bus_unref(m_bus);
        m_bus = nullptr;
        setfd(-1);
    }
}

/*
 * Other threads
 */
void
TermScoper::createScope(TermInstance *term, int fd, int pid)
{
    Tsq::Uuid scopeId;
    ScopeInfo *info = new ScopeInfo;
    info->term = term;
    info->termId = term->id();
    info->startingPid = pid;
    info->waitFd = fd;

    Lock lock(this);

    auto &prev = m_terms[term];
    if (prev) {
        stageWork(ScoperAbandon, prev);
        scopeId.generate();
    } else {
        scopeId = term->id();
    }

    info->unitName = scopeId.str();
    info->unitName.insert(0, ABBREV_NAME "-", sizeof(ABBREV_NAME));
    info->unitName.append(".scope", 6);

    stageWork(ScoperCreate, prev = info);
    commitWork();
}

void
TermScoper::unregisterTerm(TermInstance *term)
{
    FlexLock lock(this);

    auto i = m_terms.find(term);
    if (i != m_terms.end()) {
        auto prev = i->second;
        m_terms.erase(i);
        sendWorkAndUnlock(lock, ScoperAbandon, prev);
    }
}

/*
 * This thread
 */
void
TermScoper::handleJob(const char *unitName, const char *result)
{
    auto i = m_units.find(unitName);
    if (i == m_units.end())
        return;

    ScopeInfo *info = i->second;
    bool success = false;

    if (!strcmp(result, "done")) {
        Lock lock(this);

        if (m_terms.count(info->term) && info->term->id() == info->termId) {
            // two locks held
            info->term->commandSetAttribute(Tsq::attr_SCOPE_NAME, info->unitName);
            success = true;
        }
    }

    // Allow osForkTerminal to proceed to exec
    close(info->waitFd);
    info->waitFd = -1;

    if (success) {
        // TODO set up monitoring
    }
}

bool
TermScoper::handleFd()
{
    for (;;) {
        int rc = sd_bus_process(m_bus, NULL);
        if (rc == 0)
            break;
        if (rc < 0) {
            closeBus();
            break;
        }
    }
    return true;
}

void
TermScoper::handleCreate(ScopeInfo *info)
{
    const char *unitName = info->unitName.c_str();
    sd_bus_message *m;
    int rc;

    LOGDBG("Scoper: creating scope %s\n", unitName);

    rc = sd_bus_message_new_method_call(m_bus, &m,
                                        "org.freedesktop.systemd1",
                                        "/org/freedesktop/systemd1",
                                        "org.freedesktop.systemd1.Manager",
                                        "StartTransientUnit");
    if (rc < 0)
        goto err;

    sd_bus_message_append(m, "ss", unitName, "fail");
    sd_bus_message_open_container(m, 'a', "(sv)");
    sd_bus_message_append(m, "(sv)", "Description", "s", FRIENDLY_NAME " Terminal");
    sd_bus_message_append(m, "(sv)", "PIDs", "au", 1, info->startingPid);
    sd_bus_message_close_container(m);
    sd_bus_message_append(m, "a(sa(sv))", 0);

    rc = sd_bus_call_async(m_bus, NULL, m, NULL, NULL, 0);
    sd_bus_message_unref(m);
    if (rc >= 0) {
        m_units[info->unitName] = info;
        return;
    }
err:
    close(info->waitFd);
    info->waitFd = -1;
}

void
TermScoper::handleAbandon(ScopeInfo *info)
{
    const char *unitName = info->unitName.c_str();
    char *unitPath = nullptr;

    LOGDBG("Scoper: abandoning scope %s\n", unitName);

    // TODO tear down monitoring

    sd_bus_path_encode("/org/freedesktop/systemd1/unit", unitName, &unitPath);
    sd_bus_call_method_async(m_bus, NULL,
                             "org.freedesktop.systemd1",
                             unitPath,
                             "org.freedesktop.systemd1.Scope",
                             "Abandon",
                             NULL, NULL, NULL);
    free(unitPath);

    m_units.erase(info->unitName);
    if (info->waitFd != -1)
        close(info->waitFd);
    delete info;
}

bool
TermScoper::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case ScoperCreate:
        handleCreate((ScopeInfo*)item.value);
        break;
    case ScoperAbandon:
        handleAbandon((ScopeInfo*)item.value);
        break;
    default:
        return false;
    }

    return true;
}

extern "C" int
matchJobRemoved(sd_bus_message *m, void *arg, sd_bus_error *)
{
    const char *unit, *result;
    int rc = sd_bus_message_read(m, "uoss", NULL, NULL, &unit, &result);

    if (rc >= 0)
        static_cast<TermScoper*>(arg)->handleJob(unit, result);

    return 0;
}

void
TermScoper::openBus()
{
    sd_bus_open_user(&m_bus);
    int rc;

    rc = sd_bus_match_signal_async(m_bus, NULL,
                                   "org.freedesktop.systemd1",
                                   "/org/freedesktop/systemd1",
                                   "org.freedesktop.systemd1.Manager",
                                   "JobRemoved",
                                   matchJobRemoved, NULL, this);
    if (rc < 0)
        goto err;

    setfd(sd_bus_get_fd(m_bus));
    LOGDBG("Scoper: connected to user bus\n");
    return;
err:
    LOGERR("Scoper: failed to connect to user bus\n");
    sd_bus_unref(m_bus);
    m_bus = nullptr;
}

void
TermScoper::threadMain()
{
    try {
        openBus();
        runDescriptorLoop();
    }
    catch (const std::exception &e) {
        LOGERR("Scoper: caught exception: %s\n", e.what());
    }

    closeBus();

    if (s_deathSignal)
        LOGDBG("Scoper: exiting on signal %d\n", s_deathSignal);
    else
        LOGDBG("Scoper: exiting\n");
}
