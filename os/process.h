// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

extern int
osFork();

extern void
osDaemonize();

struct ForkParams {
    std::string command;
    std::string env;
    std::string dir;
    bool daemon;
    bool devnull;
};

struct PtyParams: public ForkParams {
    unsigned short width;
    unsigned short height;
    int sleepTime;
    int waitFd;
    bool exitDelay;
    bool waitForFd;
};

extern int
osForkServer(const char *pathspec, bool standalone, int *pidret);

extern int
osForkMonitor(int *pidret);

extern int
osForkProcess(const ForkParams &params, int *pidret);

extern int
osForkTerminal(const PtyParams &params, int *pidret, char *pathret = nullptr);

extern int
osForkAttributes(const char *cmdline, int *pidret);

#ifdef __linux__
#   define osRenameThread(x) pthread_setname_np(pthread_self(), x);
#else /* Mac OS X */
#   define osRenameThread(x) pthread_setname_np(x);
#endif
