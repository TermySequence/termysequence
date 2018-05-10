// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "linux-common/mon-linux.h"

int
monitorInitial(void)
{
    struct mon_string str;
    struct addr_table nltab;
    char hostbuf[256];
    int nd;

    stringInit(&str);
    stringReset(&str);

    fallbackTimezone(&str);
    fallbackSysinfo(&str, hostbuf, sizeof(hostbuf));

    if ((nd = netlinkOpen(0)) >= 0) {
        addrInit(&nltab);
        netlinkProps(nd, &nltab, &str);
        addrFreeIfDebug(&nltab);
    }

    stringPrint(&str);
    stringFreeIfDebug(&str);
    return EXITCODE_SUCCESS;
}
