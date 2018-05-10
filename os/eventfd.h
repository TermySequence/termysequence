// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <unistd.h>

#ifdef __linux__
#include <sys/eventfd.h>

static inline int
osInitEvent(int fd[2])
{
    return fd[0] = fd[1] = eventfd(0, EFD_NONBLOCK|EFD_CLOEXEC);
}

static inline void
osCloseEvent(int fd[2])
{
    close(fd[0]);
}

static inline void
osSetEvent(int fd)
{
    eventfd_write(fd, 1);
}

static inline void
osClearEvent(int fd)
{
    eventfd_t unused;
    eventfd_read(fd, &unused);
}

#else
#include "os/conn.h"

static inline int
osInitEvent(int fd[2])
{
    int rc = pipe(fd);
    if (rc == 0) {
        osMakeCloexec(fd[0]);
        osMakeCloexec(fd[1]);
        osMakeNonblocking(fd[0]);
    }
    return rc;
}

static inline void
osCloseEvent(int fd[2])
{
    close(fd[0]);
    close(fd[1]);
}

static inline void
osSetEvent(int fd)
{
    char c = 0;
    write(fd, &c, 1);
}

static inline void
osClearEvent(int fd)
{
    char unused[8];
    read(fd, unused, sizeof(unused));
}

#endif
