// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "monitor.h"
#include "lib/attr.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>

static const unsigned char s_mt6[15];

unsigned
addrCategory(const unsigned char *addr, unsigned len)
{
    if (len == 4) {
        if (addr[0] == 127)
            return ADDRCAT_HOST;
        if (addr[0] == 169 && addr[1] == 254)
            return ADDRCAT_LINK;
        if (addr[0] == 192 && addr[1] == 168)
            return ADDRCAT_SITE;
        if (addr[0] == 10)
            return ADDRCAT_SITE;
        if (addr[0] == 172 && (addr[1] & 0xf0) == 16)
            return ADDRCAT_SITE;
    } else {
        if (!memcmp(addr, s_mt6, 15))
            return ADDRCAT_HOST;
        if (addr[0] == 0xfe && (addr[1] & 0xc0) == 0x80)
            return ADDRCAT_LINK;
        if ((addr[0] & 0xfe) == 0xfc)
            return ADDRCAT_SITE;
    }

    return ADDRCAT_GLOBAL;
}

void
fallbackTimezone(struct mon_string *str)
{
    char buf[1024], *ptr;
    ssize_t rc = readlink("/etc/localtime", buf, sizeof(buf) - 1);
    if (rc != -1) {
        buf[rc] = '\0';
        for (ptr = buf; *ptr; ++ptr)
            if (*ptr >= 'A' && *ptr <= 'Z') {
                stringInsert(str, TSQ_ATTR_TIMEZONE, ptr);
                break;
            }
    }
}

static void
sysinfoProp(struct mon_string *str, FILE *fh, const char *prop, const char *attr)
{
    char buf[1024];
    size_t total, len;

    while (fgets(buf, sizeof(buf), fh)) {
        total = strlen(buf);
        if (total && buf[total - 1] == '\n')
            buf[--total] = '\0';

        len = strlen(prop);
        if (strncmp(buf, prop, len))
            continue;

        if (total - len >= 2 && buf[total - 1] == '"' && buf[len] == '"') {
            buf[--total] = '\0';
            ++len;
        }

        stringInsert(str, attr, buf + len);
        break;
    }
}

int
fallbackSysinfo(struct mon_string *str, char *hostbuf, unsigned len)
{
    struct utsname uts;
    int rc = uname(&uts);
    FILE *fh;
    hostbuf[--len] = '\0';

    if (rc == 0) {
        strncpy(hostbuf, uts.nodename, len);
        stringInsert(str, TSQ_ATTR_HOST, uts.nodename);
        stringInsert(str, TSQ_ATTR_KERNEL, uts.sysname);
        stringInsert(str, TSQ_ATTR_KERNEL_RELEASE, uts.release);
        stringInsert(str, TSQ_ATTR_ARCH, uts.machine);
    }

    fh = fopen("/etc/os-release", "re");
    if (fh) {
        sysinfoProp(str, fh, "PRETTY_NAME=", TSQ_ATTR_OS_PRETTY);
        fclose(fh);
    }
    fh = fopen("/etc/machine-info", "re");
    if (fh) {
        sysinfoProp(str, fh, "ICON_NAME=", TSQ_ATTR_ICON);
        fclose(fh);
    }

    return (rc == 0);
}
