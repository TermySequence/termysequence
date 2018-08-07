// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/conn.h"
#include "lib/exception.h"

#include <sys/un.h>
#include <sys/ucred.h>
#include <unistd.h>

int
osLocalCredsCheck(int fd)
{
    struct xucred cred{};
    socklen_t len = sizeof(cred);
    uid_t uid = getuid();
    pid_t pid;

    if (getsockopt(fd, SOL_LOCAL, LOCAL_PEERCRED, &cred, &len) == -1)
        throw Tsq::ErrnoException("getsockopt", errno);
    if (cred.cr_uid != uid)
        throw Tsq::ErrnoException(EPERM);

    len = sizeof(pid);
    if (getsockopt(fd, SOL_LOCAL, LOCAL_PEERPID, &pid, &len) == -1)
        pid = -1;

    return pid;
}

bool
osLocalCreds(int fd, int &uidret, int &pidret)
{
    struct xucred cred{};
    socklen_t len = sizeof(cred);
    pid_t pid;

    if (getsockopt(fd, SOL_LOCAL, LOCAL_PEERCRED, &cred, &len) == 0) {
        uidret = cred.cr_uid;
    } else {
        uidret = pidret = -1;
        return false;
    }

    len = sizeof(pid);
    if (getsockopt(fd, SOL_LOCAL, LOCAL_PEERPID, &pid, &len) == 0)
        pidret = pid;
    else
        pidret = -1;

    return true;
}
