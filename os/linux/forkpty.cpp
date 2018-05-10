// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/forkpty.h"
#include "lib/exception.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <climits>
#include <fcntl.h>
#include <unistd.h>

int
osForkPty(const PtyParams &params, int *fdret, char *pathret)
{
    char pathbuf[PATH_MAX];
    struct stat info;
    int mfd, sfd, uid, pid;
    unsigned mode;

    mfd = open("/dev/ptmx", O_RDWR|O_NOCTTY|O_CLOEXEC|O_NONBLOCK);
    if (mfd == -1)
        goto err;

    if (ptsname_r(mfd, pathbuf, sizeof(pathbuf)) != 0)
        goto err2;
    if (stat(pathbuf, &info) != 0)
        goto err;

    uid = getuid();
    if (info.st_uid != uid && chown(pathbuf, uid, info.st_gid) != 0)
        goto err2;

    mode = S_IRUSR|S_IWUSR|(info.st_mode & S_IWGRP);
    if ((info.st_mode & 07777) != mode && chmod(pathbuf, mode) != 0)
        goto err2;

    unlockpt(mfd);

    sfd = open(pathbuf, O_RDWR|O_CLOEXEC|O_NOCTTY);
    if (sfd == -1)
        goto err2;

    switch (pid = fork()) {
    case -1:
        close(mfd);
        close(sfd);
        throw Tsq::ErrnoException("fork", errno);
    case 0:
        close(mfd);
        if (params.width || params.height)
            osResizeTerminal(sfd, params.width, params.height);
        *fdret = sfd;
        return 0;
    default:
        close(sfd);
        *fdret = mfd;
        if (pathret)
            strcpy(pathret, pathbuf);

        return pid;
    }
err2:
    close(mfd);
err:
    throw Tsq::ErrnoException("forkpty", errno);
}
