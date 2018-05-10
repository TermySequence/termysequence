// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <syslog.h>

#ifdef NDEBUG
#define LOGDBG(x...)
#define BACKTRACE()
#else // NDEBUG
#define LOGDBG(x...) syslog(LOG_DEBUG, x)
#define BACKTRACE() osDumpBacktrace()

extern void
osDumpBacktrace();
#endif // NDEBUG

#define LOGINF(x...) syslog(LOG_INFO, x)
#define LOGNOT(x...) syslog(LOG_NOTICE, x)
#define LOGWRN(x...) syslog(LOG_WARNING, x)
#define LOGERR(x...) syslog(LOG_ERR, x)
#define LOGCRT(x...) syslog(LOG_CRIT, x)

extern void
osInitLogging(const char *name, int facility);

extern void
osCloseLogging();
