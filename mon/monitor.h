// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <errno.h>

#define EXITCODE_SUCCESS            0
#define EXITCODE_ARGPARSE           1
#define EXITCODE_HOSTNAME           2
#define EXITCODE_FAILURE            3

/* string builder */
struct mon_string {
    char *buf;
    unsigned bufsize;
    unsigned bufcap;

    char **map;
    unsigned mapsize;
    unsigned mapcap;
};

extern void stringInit(struct mon_string *str);
extern void stringReset(struct mon_string *str);
extern void stringInsert(struct mon_string *str, const char *key, const char *val);
extern void stringRemove(struct mon_string *str, const char *key);
extern void stringPrint(struct mon_string *str);
extern void stringFree(struct mon_string *str);

/* timeout tracker */
struct timeout_tracker {
    int64_t *timeouts;
    unsigned ntimeouts;
};

extern void ttInit(struct timeout_tracker *tt, unsigned count);
extern int ttNextTimeout(const struct timeout_tracker *tt, int64_t now);
extern void ttSetTimeout(struct timeout_tracker *tt, unsigned index, int64_t time);
extern void ttClearTimeout(struct timeout_tracker *tt, unsigned index);
extern int ttReady(const struct timeout_tracker *tt, unsigned index, int64_t now);

/* address category */
#define ADDRCAT_GLOBAL  0
#define ADDRCAT_SITE    1
#define ADDRCAT_LINK    2
#define ADDRCAT_HOST    3

extern unsigned addrCategory(const unsigned char *addr, unsigned len);
extern int fallbackSysinfo(struct mon_string *str, char *buf, unsigned len);
extern void fallbackTimezone(struct mon_string *str);

/* monitor entry points */
extern int monitorInitial(void);
extern int monitorMonitor(void);

#ifdef NDEBUG
#define stringFreeIfDebug(x)
#define ttFreeIfDebug(x)
#else
#define stringFreeIfDebug(x) stringFree(x)
#define ttFreeIfDebug(x) free((x)->timeouts)
#endif
