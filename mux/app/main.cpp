// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "args.h"
#include "run.h"
#include "base/threadbase.h"
#include "os/fd.h"
#include "os/signal.h"
#include "os/logging.h"
#include "os/time.h"
#include "os/eventfd.h"
#include "os/limits.h"
#include "os/locale.h"
#include "lib/exitcode.h"

#include <exception>
#include <cstdio>
#include <unistd.h>

int
main(int argc, char **argv)
{
    if (getuid() != geteuid() || getgid() != getegid()) {
        fputs("Refusing to run setuid/setgid\n", stderr);
        return EXITCODE_ARGPARSE;
    }

    osInitLocale();
    osInitMonotime();

    ArgParser args;

    try {
        args.parse(argc, argv);
        g_args = &args;
    } catch (const std::exception &e) {
        fprintf(stderr, "%s\n", e.what());
        return EXITCODE_ARGPARSE;
    }

    if (args.isConnector()) {
        try {
            return runConnector();
        } catch (const std::exception &e) {
            return EXITCODE_CONNECTERR;
        }
    }
    if (args.isQuery()) {
        return runQuery();
    }

    if (args.fdpurge())
        osPurgeFileDescriptors(SERVER_NAME);

    osInitLogging(SERVER_NAME, LOG_USER);
    osSetupServerSignalHandlers();
    osAdjustLimits();

    try {
        runConnect();
    } catch (const std::exception &e) {
        LOGERR("%s\n", e.what());
        return EXITCODE_CONNECTERR;
    }

    try {
        runListen();
    } catch (const std::exception &e) {
        LOGERR("%s\n", e.what());
        return EXITCODE_LISTENERR;
    }

    try {
        return runServer();
    } catch (const std::exception &e) {
        LOGERR("%s\n", e.what());
        return EXITCODE_SERVERERR;
    }
}

extern "C" void
deathHandler(int signal)
{
    ThreadBase::s_deathSignal = signal;
}

extern "C" void
reloadHandler(int signal)
{
    osSetEvent(ThreadBase::s_reloadFd[1]);
}
