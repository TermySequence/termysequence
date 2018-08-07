// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <sys/types.h>
#include <sys/socket.h>

extern void
osMakeNonblocking(int fd);

extern void
osMakeBlocking(int fd);

extern void
osMakeCloexec(int fd);

extern int
osLocalConnect(const char *path);

extern int
osLocalListen(const char *path, int backlog = 5);

extern void
osLocalReceiveFd(int fd, int *payload, int n);

extern int
osLocalSendFd(int fd, const int *payload, int n);

extern int
osLocalCredsCheck(int fd);

extern bool
osLocalCreds(int fd, int &uidret, int &pidret);

extern int
osAccept(int listensd, struct sockaddr *buf, socklen_t *buflen);

extern void
osWaitForWritable(int fd);

extern void
osSocketPair(int sd[2], bool bothCloexec = false);

extern int
osSocket(int domain, int type, int protocol);

extern int
osPipe(int fd[2]);
