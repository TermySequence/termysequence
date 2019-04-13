// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/sysinfo.h"

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <mach-o/dyld.h>

int
osGetCpuCores()
{
    char buf[16];
    FILE *p = popen("sysctl -n machdep.cpu.core_count", "r");
    int rc = 0;

    if (p) {
        if (fgets(buf, sizeof(buf), p))
            rc = atoi(buf);
        pclose(p);
    }

    return rc ? rc : 1;
}

int
osGetLoadAverage(char *buf, unsigned buflen)
{
    FILE *p = popen("sysctl -n vm.loadavg", "r");
    if (!p) {
        return 0;
    }
    if (!fgets(buf, buflen, p)) {
        pclose(p);
        return 0;
    }
    pclose(p);

    // Strip braces
    size_t n = strlen(buf);
    if (n == 0)
        return 0;

    size_t start = 0;
    size_t end = n - 1;

    while (start < n && (buf[start] == '{' || isspace((unsigned char)buf[start])))
        ++start;
    while (end > 0 && (buf[end] == '}' || isspace((unsigned char)buf[end])))
        --end;

    if (start >= end)
        return 0;

    memmove(buf, buf + start, end - start);
    buf[end - start] = '\0';
    return 1;
}

const char *
osGetStdlibName(char *buf16)
{
    return "darwin";
}

const char *
osGetStdlibVersion(char *buf16)
{
    int32_t v = NSVersionOfRunTimeLibrary("System");
    snprintf(buf16, 16, "%d.%d.%d", v >> 16, (v >> 8) & 255, v & 255);
    return buf16;
}
