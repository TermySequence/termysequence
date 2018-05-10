// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "monitor.h"

void
ttInit(struct timeout_tracker *tt, unsigned count)
{
    tt->timeouts = malloc(count * sizeof(int64_t));
    tt->ntimeouts = count;
    do {
        tt->timeouts[--count] = -1;
    } while (count);
}

int
ttNextTimeout(const struct timeout_tracker *tt, int64_t now)
{
    int64_t min = INT64_MAX;
    int found = 0;
    unsigned i = tt->ntimeouts;

    do {
        int64_t cur = tt->timeouts[--i];
        if (cur >= 0) {
            min = (cur < min) ? cur : min;
            found = 1;
        }
    } while (i);

    return found ? (min > now ? min - now : 0) : -1;
}

void
ttSetTimeout(struct timeout_tracker *tt, unsigned index, int64_t time)
{
    tt->timeouts[index] = time;
}

void
ttClearTimeout(struct timeout_tracker *tt, unsigned index)
{
    tt->timeouts[index] = -1;
}

int
ttReady(const struct timeout_tracker *tt, unsigned index, int64_t now)
{
    return (tt->timeouts[index] >= 0) && (tt->timeouts[index] <= now);
}
