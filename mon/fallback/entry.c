// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "monitor.h"
#include "lib/attr.h"
#include "os/time.h"
#include "os/sysinfo.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>

struct mon_state {
    struct mon_string str;

    struct timeout_tracker tt;
    int64_t loadavg_ms;
    unsigned loadavg_cores;
};

enum ttTimers { IO_TIMER, NTIMERS };

//
// Monitor
//
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
    sprintf(cores, " %u", state->loadavg_cores);

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

int
monitorMonitor(void)
{
    struct mon_state state;
    struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
    int64_t now;
    int rc;

    stringInit(&state.str);
    ttInit(&state.tt, NTIMERS);

    osInitMonotime();
    now = osMonotime();

    do {
        rc = poll(&pfd, 1, ttNextTimeout(&state.tt, now));
        now = osMonotime();

        switch (rc) {
        case -1:
            rc = (errno == EINTR);
            break;
        default:
            rc = ioEvent(&state, now);
            break;
        case 0:
            rc = 1;
            if (ttReady(&state.tt, IO_TIMER, now))
                rc = workEvent(&state, now);
            break;
        }
    } while (rc);

    ttFreeIfDebug(&state.tt);
    stringFreeIfDebug(&state.str);
    return EXITCODE_SUCCESS;
}

//
// Initial
//
struct addr_rec {
    unsigned prio;
    unsigned len;
    unsigned char addr[16];
};

static int
addrCompare(const void *aptr, const void *bptr)
{
    const struct addr_rec *a = aptr;
    const struct addr_rec *b = bptr;

    return (a->prio > b->prio) - (a->prio < b->prio);
}

static void
fallbackAddr(struct mon_string *str, const char *hostname)
{
    char buf[64];
    struct addrinfo *a, *p;
    struct addr_rec *tab;
    unsigned i, n;

    if (getaddrinfo(hostname, NULL, NULL, &a) != 0)
        return;

    for (p = a, n = 0; p; p = p->ai_next, ++n);
    tab = malloc(n * sizeof(struct addr_rec));

    for (p = a, i = 0; p; p = p->ai_next)
    {
        struct addr_rec *rec = tab + i;
        switch (p->ai_family) {
        case AF_INET:
            rec->prio = 0;
            rec->len = 4;
            memcpy(rec->addr, &((struct sockaddr_in*)p->ai_addr)->sin_addr, 4);
            ++i;
            break;
        case AF_INET6:
            rec->prio = 1;
            rec->len = 16;
            memcpy(rec->addr, &((struct sockaddr_in6*)p->ai_addr)->sin6_addr, 16);
            ++i;
            break;
        }
    }

    freeaddrinfo(a);
    n = i;

    for (i = 0; i < n; ++i) {
        struct addr_rec *rec = tab + i;
        rec->prio |= (addrCategory(rec->addr, rec->len) << 8);
    }

    qsort(tab, n, sizeof(struct addr_rec), &addrCompare);

    // Set name to top address
    if (n > 0) {
        int af = (tab->len == 4) ? AF_INET : AF_INET6;
        inet_ntop(af, tab->addr, buf, sizeof(buf));
        stringInsert(str, TSQ_ATTR_NAME, buf);
    }

    // Find first ipv4 address
    for (i = 0; i < n; ++i) {
        const struct addr_rec *rec = tab + i;
        if (rec->len == 4) {
            inet_ntop(AF_INET, rec->addr, buf, sizeof(buf));
            stringInsert(str, TSQ_ATTR_NET_IP4_ADDR, buf);
            break;
        }
    }

    // Find first ipv6 address
    for (i = 0; i < n; ++i) {
        const struct addr_rec *rec = tab + i;
        if (rec->len == 16) {
            inet_ntop(AF_INET6, rec->addr, buf, sizeof(buf));
            stringInsert(str, TSQ_ATTR_NET_IP6_ADDR, buf);
            break;
        }
    }

    free(tab);
}

int
monitorInitial(void)
{
    struct mon_string str;
    char hostbuf[256];
    int rc = EXITCODE_HOSTNAME;

    stringInit(&str);
    stringReset(&str);

    if (fallbackSysinfo(&str, hostbuf, sizeof(hostbuf))) {
        fallbackAddr(&str, hostbuf);
        rc = EXITCODE_SUCCESS;
    }

    fallbackTimezone(&str);

    stringPrint(&str);
    stringFreeIfDebug(&str);
    return rc;
}
