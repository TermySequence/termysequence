// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/status.h"
#include "lib/attrstr.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <libproc.h>

void
osStatusInit(int **)
{
}

void
osStatusTeardown(int *)
{
}

void
osGetProcessAttributes(int *, int pid,
                       std::unordered_map<std::string,std::string> &current,
                       std::unordered_map<std::string,std::string> &next)
{
    struct proc_taskallinfo tai;
    struct proc_vnodepathinfo vpi;
    char path[PROC_PIDPATHINFO_MAXSIZE] = { 0 };

    int rc = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &tai, sizeof(tai));
    if (rc >= sizeof(tai)) {
        std::string str = std::to_string(tai.pbsd.pbi_uid);
        if (current[Tsq::attr_PROC_UID] != str)
            next[Tsq::attr_PROC_UID] = str;

        str = std::to_string(tai.pbsd.pbi_gid);
        if (current[Tsq::attr_PROC_GID] != str)
            next[Tsq::attr_PROC_GID] = str;
    }

    rc = proc_pidinfo(pid, PROC_PIDVNODEPATHINFO, 0, &vpi, sizeof(vpi));
    if (rc >= sizeof(vpi)) {
        const char *ptr = vpi.pvi_cdir.vip_path;
        std::string str(ptr, strnlen(ptr, MAXPATHLEN));
        if (current[Tsq::attr_PROC_CWD] != str)
            next[Tsq::attr_PROC_CWD] = str;
    }

    rc = proc_name(pid, path, sizeof(path));
    if (rc > 0) {
        std::string str(path, strnlen(path, sizeof(path)));
        if (current[Tsq::attr_PROC_COMM] != str)
            next[Tsq::attr_PROC_COMM] = str;
    }

    int mib[] = { CTL_KERN, KERN_PROCARGS2, pid };
    size_t size;

    rc = sysctl(mib, 3, NULL, &size, NULL, 0);
    if (rc == 0) {
        char *ptr, *buf = new char[++size];

        rc = sysctl(mib, 3, buf, &size, NULL, 0);
        if (rc == 0 && size > sizeof(int)) {
            int argc;
            memcpy(&argc, buf, sizeof(int));
            ptr = buf + sizeof(int);
            size -= sizeof(int);

            std::string str(ptr, strnlen(ptr, size));
            if (current[Tsq::attr_PROC_EXE] != str)
                next[Tsq::attr_PROC_EXE] = str;

            ptr += str.size();
            size -= str.size();
            str.clear();

            while (size > 0 && argc > 0) {
                if (*ptr) {
                    std::string arg(ptr, strnlen(ptr, size));
                    str.append(arg);
                    str.push_back('\x1f');

                    ptr += arg.size();
                    size -= arg.size();
                    --argc;
                } else {
                    ++ptr;
                    --size;
                }
            }

            if (!str.empty())
                str.pop_back();
            if (current[Tsq::attr_PROC_ARGV] != str)
                next[Tsq::attr_PROC_ARGV] = str;
        }

        delete [] buf;
    }
}
