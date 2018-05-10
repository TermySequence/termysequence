// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "logging.h"

#ifdef NDEBUG
#define LOGGING_FLAGS 0
#else
#define LOGGING_FLAGS LOG_PERROR
#endif

void
osInitLogging(const char *name, int facility)
{
    openlog(name, LOGGING_FLAGS, facility);
}

void
osCloseLogging()
{
    // NOTE: not currently called
    // closelog();
}

#ifndef NDEBUG
#include <execinfo.h>
#include <unistd.h>

void
osDumpBacktrace()
{
    void *buf[64];
    int rc = backtrace(buf, 64);
    backtrace_symbols_fd(buf, rc, STDERR_FILENO);
}
#endif
