// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/forkpty.h"
#include "os/conn.h"
#include "lib/exception.h"

#include <unistd.h>
#include <termios.h>
#include <util.h>

int
osForkPty(const PtyParams &params, int *fdret, char *pathret)
{
    struct winsize winsize = { params.height, params.width, 0, 0 };
    bool havesize = params.width || params.height;
    int mfd = -1, sfd = -1;

    int pid = openpty(&mfd, &sfd, pathret, NULL, havesize ? &winsize : NULL);
    osMakeCloexec(mfd);
    osMakeCloexec(sfd);
    if (pid == -1)
        throw Tsq::ErrnoException("openpty", errno);

    switch (pid = fork()) {
    case -1:
        close(mfd);
        close(sfd);
        throw Tsq::ErrnoException("fork", errno);
    case 0:
        close(mfd);
        *fdret = sfd;
        return 0;
    default:
        close(sfd);
        osMakeNonblocking(mfd);
        *fdret = mfd;
        return pid;
    }
}
