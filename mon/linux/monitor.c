// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "linux-common/mon-linux.h"
#include "lib/attr.h"
#include "os/time.h"
#include "os/sysinfo.h"

#include <stdio.h>
#include <unistd.h>
#include <poll.h>

struct mon_state {
    struct pollfd pfd[2];

    struct mon_string str;
    struct addr_table nltab;
    int nd2;

    struct timeout_tracker tt;
    int64_t loadavg_ms;
    unsigned loadavg_cores;
};

enum ttTimers { IO_TIMER, NL_TIMER, NTIMERS };

static int
ioEvent(struct mon_state *state, int64_t now)
{
    char buf[256];

    if (!fgets(buf, sizeof(buf), stdin)) {
        return 0;
    } else if (!strcmp(buf, TSQ_ATTR_LOADAVG "\n")) {
        ttSetTimeout(&state->tt, IO_TIMER, now);
        state->loadavg_ms = now + 1800000;
        state->loadavg_cores = osGetCpuCores();
    }

    return 1;
}

static int
workEvent(struct mon_state *state, int64_t now)
{
    char buf[256], cores[16];
    sprintf(buf, " %u", state->loadavg_cores);

    stringReset(&state->str);

    if (now < state->loadavg_ms && osGetLoadAverage(buf, 240)) {
        strcat(buf, cores);
        stringInsert(&state->str, TSQ_ATTR_LOADAVG, buf);
        ttSetTimeout(&state->tt, IO_TIMER, now + 10000);
    }
    else {
        stringRemove(&state->str, TSQ_ATTR_LOADAVG);
        ttClearTimeout(&state->tt, IO_TIMER);
    }

    stringPrint(&state->str);
    return 1;
}

static void
netlinkClose(struct mon_state *state)
{
    close(state->pfd[1].fd);
    state->pfd[1].fd = -1;
    close(state->nd2);
    ttClearTimeout(&state->tt, NL_TIMER);
}

static int
netlinkEvent(struct mon_state *state, int64_t now)
{
    if (netlinkFlush(state->pfd[1].fd) == 0) {
        ttSetTimeout(&state->tt, NL_TIMER, now + 1000);
    } else {
        netlinkClose(state);
    }
    return 1;
}

static void
netlinkDelay(struct mon_state *state)
{
    stringReset(&state->str);
    addrReset(&state->nltab);

    if (netlinkProps(state->nd2, &state->nltab, &state->str) == 0) {
        stringPrint(&state->str);
        ttClearTimeout(&state->tt, NL_TIMER);
    } else {
        netlinkClose(state);
    }
}

int
monitorMonitor(void)
{
    struct mon_state state;
    int64_t now;
    int rc;

    stringInit(&state.str);
    addrInit(&state.nltab);
    ttInit(&state.tt, NTIMERS);

    state.pfd[0].fd = STDIN_FILENO;
    state.pfd[0].events = POLLIN;
    state.pfd[1].fd = netlinkOpen(1);
    state.pfd[1].events = POLLIN;
    state.nd2 = netlinkOpen(0);

    if (state.nd2 < 0)
        state.pfd[1].fd = -1;

    osInitMonotime();
    now = osMonotime();

    do {
        rc = poll(state.pfd, 2, ttNextTimeout(&state.tt, now));
        now = osMonotime();

        switch (rc) {
        case -1:
            rc = (errno == EINTR);
            break;
        default:
            rc = (state.pfd[1].revents ? netlinkEvent : ioEvent)(&state, now);
            break;
        case 0:
            rc = 1;
            if (ttReady(&state.tt, NL_TIMER, now))
                netlinkDelay(&state);
            if (ttReady(&state.tt, IO_TIMER, now))
                rc = workEvent(&state, now);
            break;
        }
    } while (rc);

    ttFreeIfDebug(&state.tt);
    addrFreeIfDebug(&state.nltab);
    stringFreeIfDebug(&state.str);
    return EXITCODE_SUCCESS;
}
