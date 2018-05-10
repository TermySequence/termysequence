// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "linux-sdbus/mon-sdbus.h"
#include "linux-common/mon-linux.h"
#include "lib/attr.h"
#include "os/sysinfo.h"
#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <systemd/sd-event.h>

static int
ioEvent(sd_event_source *unused, int fd, uint32_t rev, void *s)
{
    struct mon_state *state = s;
    char buf[256];

    if (!fgets(buf, sizeof(buf), stdin)) {
        sd_event_exit(state->event, 0);
    } else if (!strcmp(buf, TSQ_ATTR_LOADAVG "\n")) {
        sd_event_now(state->event, CLOCK_MONOTONIC, &state->loadavg_usec);
        sd_event_source_set_time(state->workTimer, state->loadavg_usec);
        sd_event_source_set_enabled(state->workTimer, SD_EVENT_ON);
        state->loadavg_usec += 1800000000;
        state->loadavg_cores = osGetCpuCores();
    }

    return 0;
}

static int
workEvent(sd_event_source *unused, uint64_t usec, void *s)
{
    struct mon_state *state = s;
    char buf[256], cores[16];
    sprintf(cores, " %u", state->loadavg_cores);

    stringReset(&state->str);

    if (usec < state->loadavg_usec && osGetLoadAverage(buf, 240)) {
        strcat(buf, cores);
        stringInsert(&state->str, TSQ_ATTR_LOADAVG, buf);
        usec += 10000000;
    }
    else {
        stringRemove(&state->str, TSQ_ATTR_LOADAVG);
        usec = UINT64_MAX;
    }

    stringPrint(&state->str);
    sd_event_source_set_time(state->workTimer, usec);
    return 0;
}

static inline void
netlinkClose(struct mon_state *state)
{
    close(state->nd1);
    state->nd1 = -1;
    close(state->nd2);
    state->nd2 = -1;
}

static int
netlinkEvent(sd_event_source *unused, int fd, uint32_t rev, void *s)
{
    struct mon_state *state = s;
    uint64_t now;

    int rc = netlinkFlush(state->nd1);
    if (rc < 0)
        netlinkClose(state);

    sd_event_now(state->event, CLOCK_MONOTONIC, &now);
    sd_event_source_set_time(state->netTimer, now + 1000000);
    sd_event_source_set_enabled(state->netTimer, SD_EVENT_ONESHOT);
    return rc;
}

static int
netlinkDelay(sd_event_source *unused, uint64_t usec, void *s)
{
    struct mon_state *state = s;
    int rc;

    stringReset(&state->str);
    addrReset(&state->nltab);

    rc = netlinkProps(state->nd2, &state->nltab, &state->str);
    if (rc < 0)
        netlinkClose(state);

    stringPrint(&state->str);
    return rc;
}

static int
hostnamePropertiesChanged(sd_bus_message *unused, void *s, sd_bus_error *error)
{
    struct mon_state *state = s;
    stringReset(&state->str);
    hostnameProps(state);
    stringPrint(&state->str);
    return 0;
}

static int
timedatePropertiesChanged(sd_bus_message *unused, void *s, sd_bus_error *error)
{
    struct mon_state *state = s;
    stringReset(&state->str);
    timedateProps(state);
    stringPrint(&state->str);
    return 0;
}

int
monitorMonitor(void)
{
    struct mon_state state = {};
    sd_event *event = NULL;
    int rc;

    sd_event_default(&event);

    state.event = event;
    state.nd1 = netlinkOpen(1);
    state.nd2 = netlinkOpen(0);

    rc = sd_bus_open_system(&state.bus);
    if (rc == 0) {
        sd_bus_add_match(state.bus, NULL,
                         "type='signal',"
                         "sender='org.freedesktop.hostname1',"
                         "interface='org.freedesktop.DBus.Properties',"
                         "member='PropertiesChanged',"
                         "arg0='org.freedesktop.hostname1'",
                         hostnamePropertiesChanged,
                         &state);

        sd_bus_add_match(state.bus, NULL,
                         "type='signal',"
                         "sender='org.freedesktop.timedate1',"
                         "interface='org.freedesktop.DBus.Properties',"
                         "member='PropertiesChanged',"
                         "arg0='org.freedesktop.timedate1'",
                         timedatePropertiesChanged,
                         &state);

        sd_bus_attach_event(state.bus, event, 0);
    }

    if (state.nd1 >= 0 && state.nd2 >= 0) {
        sd_event_add_io(event, NULL, state.nd1, EPOLLIN, &netlinkEvent, &state);
        sd_event_add_time(event, &state.netTimer, CLOCK_MONOTONIC,
                          UINT64_MAX, 0, &netlinkDelay, &state);
    }

    sd_event_add_io(event, NULL, STDIN_FILENO, EPOLLIN, &ioEvent, &state);
    sd_event_add_time(event, &state.workTimer, CLOCK_MONOTONIC,
                      UINT64_MAX, 0, &workEvent, &state);

    stringInit(&state.str);
    addrInit(&state.nltab);

    startingProps(&state);
    rc = sd_event_loop(event);

    addrFreeIfDebug(&state.nltab);
    stringFreeIfDebug(&state.str);
    return (rc == 0) ? EXITCODE_SUCCESS : EXITCODE_FAILURE;
}
