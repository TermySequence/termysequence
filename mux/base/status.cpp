// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "status.h"
#include "app/args.h"
#include "os/status.h"
#include "os/process.h"
#include "os/pty.h"
#include "os/wait.h"
#include "os/signal.h"
#include "lib/flags.h"
#include "lib/enums.h"
#include "lib/attrstr.h"
#include "lib/base64.h"
#include "lib/wire.h"

#include <cstdio>
#include <unistd.h>

#define TR_OUTCOME1 TL("server", "Process %1 killed by signal %2 (core dumped)", "outcome1")
#define TR_OUTCOME2 TL("server", "Process %1 killed by signal %2", "outcome2")
#define TR_OUTCOME3 TL("server", "Process %1 exited with status %2", "outcome3")

TermStatusTracker::TermStatusTracker(const Translator *translator,
                                     SharedStringMap &&environ) :
    termiosFlags{},
    termiosChars{},
    m_environ(std::move(environ)),
    m_translator(translator)
{
    osStatusInit(&p_os);
}

TermStatusTracker::TermStatusTracker() :
    TermStatusTracker(g_args->defaultTranslator(), nullptr)
{}

TermStatusTracker::~TermStatusTracker()
{
    osStatusTeardown(p_os);
}

inline void
TermStatusTracker::setTermiosInfo()
{
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    termiosFlags[0] = __builtin_bswap32(termiosFlags[0]);
    termiosFlags[1] = __builtin_bswap32(termiosFlags[1]);
    termiosFlags[2] = __builtin_bswap32(termiosFlags[2]);
    #endif

    char buf[43];
    base64(reinterpret_cast<char*>(termiosFlags), sizeof(termiosFlags), buf);
    base64(termiosChars, sizeof(termiosChars), buf + 16);
    m_changed[Tsq::attr_PROC_TERMIOS].assign(buf, sizeof(buf));
}

void
TermStatusTracker::updateOnce(int fd, int primary, const char *title)
{
    osGetProcessAttributes(p_os, primary, m_all, m_changed);
    if (osGetTerminalAttributes(fd, termiosFlags, termiosChars))
        setTermiosInfo();

    m_changed[Tsq::attr_PROC_STATUS] = std::to_string(Tsq::TermBusy);
    m_changed[Tsq::attr_PROC_PID] = std::to_string(primary);
    m_changed[Tsq::attr_SESSION_TITLE] = title;
    m_changed[Tsq::attr_SESSION_TITLE2] = title;
}

bool
TermStatusTracker::update(int fd, int primary)
{
    unsigned next_status;
    int next_pid;
    uint32_t next_termiosFlags[3] = {};
    char next_termiosChars[20] = {};

    m_changed.clear();

    if (fd == -1) {
        next_status = Tsq::TermClosed;
        next_pid = primary;
        goto skip;
    }

    next_pid = tcgetpgrp(fd);

    if (next_pid == 0)
        next_status = Tsq::TermIdle;
    else if (next_pid == primary)
        next_status = Tsq::TermActive;
    else
        next_status = Tsq::TermBusy;

    osGetTerminalAttributes(fd, next_termiosFlags, next_termiosChars);

    if (next_pid)
        osGetProcessAttributes(p_os, next_pid, m_all, m_changed);

skip:
    if (status != next_status) {
        status = next_status;
        m_changed[Tsq::attr_PROC_STATUS] = std::to_string(next_status);
    }
    if (pid != next_pid) {
        pid = next_pid;
        m_changed[Tsq::attr_PROC_PID] = std::to_string(next_pid);
    }
    if (memcmp(termiosFlags, next_termiosFlags, sizeof(termiosFlags)) ||
        memcmp(termiosChars, next_termiosChars, sizeof(termiosChars)))
    {
        memcpy(termiosFlags, next_termiosFlags, sizeof(termiosFlags));
        memcpy(termiosChars, next_termiosChars, sizeof(termiosChars));
        setTermiosInfo();
    }

    for (const auto &i: m_changed)
        m_all[i.first] = i.second;

    return !m_changed.empty();
}

void
TermStatusTracker::handleEnvRule(const char *rule, std::string &envbuf)
{
    if (*rule == '@') {
        auto i = m_environ->find(++rule);
        if (i != m_environ->end()) {
            m_sessenv[i->first] = std::make_pair(true, i->second);
            envbuf += '+';
            envbuf += i->first;
            envbuf += '=';
            envbuf += i->second;
        } else {
            m_sessenv[rule].first = false;
            envbuf += '-';
            envbuf += rule;
        }
        envbuf += '\0';
    }
}

void
TermStatusTracker::start(ForkParams *params)
{
    status = Tsq::TermIdle;
    pid = 0;
    m_outcome = Tsq::TermRunning;
    m_exitcode = 0;
    m_outcomeStr.clear();

    memset(termiosFlags, 0, sizeof(termiosFlags));
    memset(termiosChars, 0, sizeof(termiosChars));

    // Set attributes
    m_all.clear();
    m_all.emplace(Tsq::attr_PROC_CWD, params->dir);

    std::string tmp(params->command);
    for (char &c: tmp)
        if (c == '\0')
            c = '\x1f';

    m_all.emplace(Tsq::attr_PROC_ARGV, std::move(tmp));

    const char *comm = params->command.c_str();
    const char *ptr = strrchr(comm, '/');
    m_all.emplace(Tsq::attr_PROC_COMM, ptr ? ptr + 1 : comm);

    // Prepare the environment
    const char *envc = params->env.c_str();
    tmp.assign(1, '\0');
    m_sessenv.clear();

    handleEnvRule(envc, tmp);
    for (size_t i = 0; i < params->env.size(); ++i)
        if (envc[i] == '\0')
            handleEnvRule(envc + i + 1, tmp);

    params->env.append(tmp);
    params->env.append("+" ENV_NAME "=" PROJECT_VERSION);

    // Build list of session environment variable names
    tmp.clear();
    for (const auto &elt: m_sessenv) {
        tmp += elt.first;
        tmp += '\x1f';
    }
    if (!tmp.empty())
        tmp.pop_back();

    m_all.emplace(Tsq::attr_ENV_NAMES, std::move(tmp));

    // Build list of current environment assignments
    tmp.clear();
    for (const auto &elt: m_sessenv) {
        if (elt.second.first) {
            tmp += elt.first;
            tmp += '=';
            tmp += elt.second.second;
            tmp += '\x1f';
        }
    }
    if (!tmp.empty())
        tmp.pop_back();

    m_all.emplace(Tsq::attr_ENV_CURRENT, tmp);
    m_all.emplace(Tsq::attr_ENV_GOAL, std::move(tmp));
    m_all.emplace(Tsq::attr_ENV_DIRTY, "0");

    m_changed = m_all;
}

bool
TermStatusTracker::setEnviron(SharedStringMap &environ)
{
    std::string dirty(1, '0');
    std::string goal;

    // See if there are any conflicts
    for (const auto &elt: m_sessenv) {
        auto i = environ->find(elt.first);
        bool exists = i != environ->end();

        if (elt.second.first != exists ||
            (exists && elt.second.second != i->second)) {
            dirty[0] = '1';
        }
        if (exists) {
            goal += elt.first;
            goal += '=';
            goal += i->second;
            goal += '\x1f';
        }
    }
    if (!goal.empty())
        goal.pop_back();

    m_environ = std::move(environ);
    m_changed.clear();

    if (m_all[Tsq::attr_ENV_DIRTY] != dirty) {
        m_all[Tsq::attr_ENV_DIRTY] = dirty;
        m_changed.emplace(Tsq::attr_ENV_DIRTY, std::move(dirty));
    }
    if (m_all[Tsq::attr_ENV_GOAL] != goal) {
        m_all[Tsq::attr_ENV_GOAL] = goal;
        m_changed.emplace(Tsq::attr_ENV_GOAL, std::move(goal));
    }
    return !m_changed.empty();
}

void
TermStatusTracker::resetEnviron()
{
    for (auto &&elt: m_sessenv) {
        auto i = m_environ->find(elt.first);
        if ((elt.second.first = i != m_environ->end()))
            elt.second.second = i->second;
    }

    std::string dirty(1, '0');
    m_changed.clear();

    m_all[Tsq::attr_ENV_DIRTY] = dirty;
    m_changed.emplace(Tsq::attr_ENV_DIRTY, std::move(dirty));

    dirty = m_all[Tsq::attr_ENV_CURRENT] = m_all[Tsq::attr_ENV_GOAL];
    m_changed.emplace(Tsq::attr_ENV_CURRENT, std::move(dirty));
}

void
TermStatusTracker::setOutcome(int pid, int disposition)
{
    if (osExitedOnSignal(disposition)) {
        m_exitcode = osExitSignal(disposition);
        if (osDumpedCore(disposition)) {
            m_outcome = Tsq::TermDumped;
            g_args->arg(m_outcomeStr, TR_OUTCOME1, pid, m_exitcode);
        } else {
            m_outcome = Tsq::TermKilled;
            g_args->arg(m_outcomeStr, TR_OUTCOME2, pid, m_exitcode);
        }
    }
    else {
        m_exitcode = osExitStatus(disposition);
        m_outcome = m_exitcode ? Tsq::TermExitN : Tsq::TermExit0;
        g_args->arg(m_outcomeStr, TR_OUTCOME3, pid, m_exitcode);
    }
}

const char *
TermStatusTracker::updateOutcome()
{
    m_changed.clear();
    m_changed[Tsq::attr_PROC_EXITCODE] = std::to_string(m_exitcode);
    m_changed[Tsq::attr_PROC_OUTCOME] = std::to_string(m_outcome);
    m_changed[Tsq::attr_PROC_OUTCOMESTR] = m_outcomeStr;
    return m_outcomeStr.c_str();
}

void
TermStatusTracker::sendSignal(int fd, int signal)
{
    if (fd != -1) {
        int pgrp = tcgetpgrp(fd);

        if (pgrp != 0)
            osKillProcess(pgrp, signal);
    }
}
