// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "status.h"
#include "app/args.h"
#include "os/status.h"
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

TermStatusTracker::TermStatusTracker(const Translator *translator) :
    termiosFlags{},
    termiosChars{},
    m_translator(translator)
{
    osStatusInit(&p_os);
}

TermStatusTracker::TermStatusTracker() :
    TermStatusTracker(g_args->defaultTranslator())
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
TermStatusTracker::start(const std::string &command, const std::string &dir)
{
    status = Tsq::TermIdle;
    pid = 0;
    m_outcome = Tsq::TermRunning;
    m_exitcode = 0;

    memset(termiosFlags, 0, sizeof(termiosFlags));
    memset(termiosChars, 0, sizeof(termiosChars));

    m_all.clear();
    m_all.emplace(Tsq::attr_PROC_CWD, dir);
    m_all.emplace(Tsq::attr_PROC_EXE, command.c_str());

    std::string tmp(command);
    for (int i = 0; i < tmp.size(); ++i)
        if (tmp[i] == '\0')
            tmp[i] = '\x1f';

    m_all.emplace(Tsq::attr_PROC_ARGV, tmp);
    m_changed = m_all;

    m_outcomeStr.clear();
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

    m_changed.clear();
    m_changed[Tsq::attr_PROC_EXITCODE] = std::to_string(m_exitcode);
    m_changed[Tsq::attr_PROC_OUTCOME] = std::to_string(m_outcome);
    m_changed[Tsq::attr_PROC_OUTCOMESTR] = m_outcomeStr;
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
