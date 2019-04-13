// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/sysinfo.h"

#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <gnu/libc-version.h>

int
osGetCpuCores()
{
    DIR *dir = opendir("/sys/devices/system/cpu");
    const struct dirent *file;
    int rc = 0;

    if (dir != NULL) {
        while ((file = readdir(dir)) != NULL) {
            const char *name = file->d_name;
            rc += !strncmp(name, "cpu", 3) &&
                strlen(name + 3) == strspn(name + 3, "0123456789");
        }
        closedir(dir);
    }

    return rc ? rc : 1;
}

int
osGetLoadAverage(char *buf, unsigned buflen)
{
    FILE *f = fopen("/proc/loadavg", "re");
    if (!f) {
        return 0;
    }
    if (!fgets(buf, buflen, f)) {
        fclose(f);
        return 0;
    }
    fclose(f);

    // Report only the first three fields
    int fieldcount = 0;
    for (size_t i = 0, n = strlen(buf); i < n; ++i) {
        if (buf[i] == ' ' && ++fieldcount == 3) {
            buf[i] = '\0';
            return 1;
        }
    }

    return 0;
}

const char *
osGetStdlibName(char *buf16)
{
    return "glibc";
}

const char *
osGetStdlibVersion(char *buf16)
{
    return gnu_get_libc_version();
}
