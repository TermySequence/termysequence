// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/conn.h"
#include "lib/exception.h"

#include <sys/un.h>
#include <unistd.h>

int
osLocalCredsCheck(int fd)
{
    struct ucred cred{};
    socklen_t len = sizeof(cred);
    uid_t uid = getuid();

    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) == -1)
        throw Tsq::ErrnoException("getsockopt", errno);
    if (cred.uid != uid)
        throw Tsq::ErrnoException(EPERM);

    return cred.pid;
}

bool
osLocalCreds(int fd, int &uidret, int &pidret)
{
    struct ucred cred{};
    socklen_t len = sizeof(cred);

    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) == 0) {
        uidret = cred.uid;
        pidret = cred.pid;
        return true;
    } else {
        uidret = pidret = -1;
        return false;
    }
}
