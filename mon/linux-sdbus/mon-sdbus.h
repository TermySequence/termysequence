// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "linux-common/mon-linux.h"
#include <systemd/sd-bus.h>

struct mon_state {
    sd_bus *bus;
    sd_event *event;
    sd_event_source *netTimer;
    sd_event_source *workTimer;

    struct mon_string str;
    struct addr_table nltab;
    int nd1, nd2;

    uint64_t loadavg_usec;
    unsigned loadavg_cores;
};

extern void startingProps(struct mon_state *state);
extern void hostnameProps(struct mon_state *state);
extern void timedateProps(struct mon_state *state);
