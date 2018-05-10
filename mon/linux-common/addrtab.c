// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "linux-common/mon-linux.h"

#include <linux/rtnetlink.h>

#define STARTING_NADDRS 16

void
addrInit(struct addr_table *tab)
{
    tab->buf = malloc(STARTING_NADDRS * sizeof(struct addr_rec));
    tab->tabsize = 0;
    tab->tabcap = STARTING_NADDRS;
}

void
addrReset(struct addr_table *tab)
{
    tab->tabsize = 0;
}

static int
insertMatch(struct addr_rec *cur, const struct addr_rec *rec)
{
    unsigned stem = (cur->stem < rec->stem) ? cur->stem : rec->stem;
    unsigned pos = 0;

    while (stem >= 8) {
        if (cur->addr[pos] != rec->addr[pos])
            return 0;
        stem -= 8;
        ++pos;
    }
    while (stem) {
        unsigned char mask = 1 << stem;
        if ((cur->addr[pos] & mask) != (rec->addr[pos] & mask))
            return 0;
        --stem;
    }

    if (cur->stem > rec->stem) {
        cur->stem = rec->stem;
        memcpy(cur->addr, rec->addr, rec->len);
    }

    return 1;
}

void
addrInsert(struct addr_table *tab, const struct addr_rec *rec)
{
    unsigned i;

    for (i = 0; i < tab->tabsize; ++i)
        if (!memcmp(tab->buf + i, rec, 8) && insertMatch(tab->buf + i, rec))
            return;

    ++tab->tabsize;
    while (tab->tabcap < tab->tabsize) {
        tab->tabcap *= 2;
        tab->buf = realloc(tab->buf, tab->tabcap);
    }

    memcpy(tab->buf + i, rec, sizeof(*rec));
}

static int
updateMatch(struct addr_rec *cur, const struct addr_rec *rec)
{
    unsigned stem = rec->stem;
    unsigned pos = 0;

    while (stem >= 8) {
        if (cur->addr[pos] != rec->addr[pos])
            return 0;
        stem -= 8;
        ++pos;
    }
    while (stem) {
        unsigned char mask = 1 << stem;
        if ((cur->addr[pos] & mask) != (rec->addr[pos] & mask))
            return 0;
        --stem;
    }

    if (cur->prio > rec->prio)
        cur->prio = rec->prio;

    return 1;
}

void
addrUpdate(struct addr_table *tab, const struct addr_rec *rec)
{
    unsigned i;

    for (i = 0; i < tab->tabsize; ++i)
        if (!memcmp(tab->buf + i, rec, 8) && updateMatch(tab->buf + i, rec))
            break;
}

int
addrCompare(const void *aptr, const void *bptr)
{
    const struct addr_rec *a = aptr;
    const struct addr_rec *b = bptr;

    if (a->prio < b->prio)
        return -1;
    if (a->prio > b->prio)
        return 1;

    return (a->iface < b->iface) ? -1 : (a->iface > b->iface);
}

void
addrAdjust(struct addr_table *tab)
{
    const unsigned char catmap[4] = {
        RT_SCOPE_UNIVERSE,
        RT_SCOPE_SITE,
        RT_SCOPE_LINK,
        RT_SCOPE_HOST
    };

    for (unsigned i = 0; i < tab->tabsize; ++i)
    {
        struct addr_rec *rec = tab->buf + i;
        unsigned category = catmap[addrCategory(rec->addr, rec->len)];

        if ((rec->prio >> 8) < category) {
            rec->prio &= 1;
            rec->prio |= (category << 8);
        }

        rec->prio <<= 16;
        rec->prio |= (category << 8);
        rec->prio |= (rec->len == 16);
    }
}
