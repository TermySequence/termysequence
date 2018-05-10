// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/time.h"
#include <time.h>

void
osInitMonotime()
{
}

int64_t
osMonotime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);

    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int64_t
osWalltime()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
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
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME_COARSE, &ts);

    return ts.tv_sec * 10 + ts.tv_nsec / 100000000 - base;
}

int64_t
osSigtime()
{
    return time(NULL);
}
