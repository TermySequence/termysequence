// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <sys/types.h>
#include <sys/select.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>

static struct termios s_saveios;
static int s_israw;

void
tty_raw(int fd)
{
    if (!s_israw) {
        struct termios buf;

        if (tcgetattr(fd, &buf) < 0)
            exit(10);

        s_saveios = buf;
        cfmakeraw(&buf);
        buf.c_cc[VMIN] = 1;
        buf.c_cc[VTIME] = 0;

        if (tcsetattr(fd, TCSAFLUSH, &buf) < 0)
            exit(11);

        s_israw = 1;
    }
}

void
tty_unraw(int fd)
{
    if (s_israw) {
        tcsetattr(fd, TCSAFLUSH, &s_saveios);
        s_israw = 0;
    }
}

void
deathHandler(int signal)
{
    tty_unraw(STDIN_FILENO);
    _exit(99);
}

void
setSignalHandler(void)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = deathHandler;
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}

void
read_replies(int fd, int timeout)
{
    char buf[1024];
    ssize_t rc;
    fd_set readset;
    struct timeval tv;

    FD_ZERO(&readset);

    while (1) {
        FD_SET(fd, &readset);
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        if (select(fd + 1, &readset, NULL, NULL, &tv) <= 0)
            return;

        rc = read(fd, buf, sizeof(buf));
        if (rc <= 0)
            return;

        fputs("Response: ", stderr);

        for (ssize_t i = 0; i < rc; ++i)
            if (isprint((unsigned char)buf[i])) {
                fprintf(stderr, "%c", buf[i]);
            }
            else {
                fprintf(stderr, "(%x)", (unsigned char)buf[i]);
            }

        fputs("\r\n", stderr);
    }
}
