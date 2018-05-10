// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "monitor.h"

/* address map */
struct addr_rec {
    uint32_t iface;
    uint32_t len;
    uint32_t prio;
    uint32_t stem;
    unsigned char addr[16];
};

struct addr_table {
    struct addr_rec *buf;
    unsigned tabsize;
    unsigned tabcap;
};

extern void addrInit(struct addr_table *tab);
extern void addrReset(struct addr_table *tab);
extern void addrInsert(struct addr_table *tab, const struct addr_rec *rec);
extern void addrUpdate(struct addr_table *tab, const struct addr_rec *rec);
extern void addrAdjust(struct addr_table *tab);
extern int addrCompare(const void *a, const void *b);

#ifdef NDEBUG
#define addrFreeIfDebug(x)
#else
#define addrFreeIfDebug(x) free((x)->buf)
#endif

/* netlink */
extern int netlinkOpen(int notifier);
extern int netlinkFlush(int nd);
extern int netlinkProps(int nd, struct addr_table *tab, struct mon_string *str);
