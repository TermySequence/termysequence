// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/time.h"
#include <sys/time.h>
#include <time.h>
#include <mach/mach_time.h>

static mach_timebase_info_data_t s_timebase_info;

void
osInitMonotime()
{
    mach_timebase_info(&s_timebase_info);
    s_timebase_info.denom *= 1000000;
}

int64_t
osMonotime()
{
    return mach_absolute_time() * s_timebase_info.numer / s_timebase_info.denom;
}

int64_t
osWalltime()
{
    struct timeval ts;
    gettimeofday(&ts, NULL);

    return ts.tv_sec * 1000 + ts.tv_usec / 1000;
}

int64_t
osBasetime(int64_t *baseret)
{
    int64_t rc = osWalltime();
    *baseret = rc / 100;
    return rc;
}

int32_t
osModtime(int64_t base)
{
    struct timeval ts;
    gettimeofday(&ts, NULL);

    return ts.tv_sec * 10 + ts.tv_usec / 100000 - base;
}

int64_t
osSigtime()
{
    return time(NULL);
}
