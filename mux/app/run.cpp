// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "run.h"
#include "args.h"
#include "base/listener.h"
#include "base/exception.h"
#include "os/dir.h"
#include "os/conn.h"
#include "os/process.h"
#include "os/logging.h"
#include "os/git.h"
#include "lib/enums.h"
#include "lib/exitcode.h"
#include "config.h"

#include <unistd.h>
#include <cstdio>

#if USE_SYSTEMD
#include <systemd/sd-daemon.h>
#else
#define sd_listen_fds(x) (-ENOTSUP)
#endif

static char s_udspath[SOCKET_PATHLEN];
static char s_pidpath[SOCKET_PATHLEN];
static int s_listenfd = 3; // SD_LISTEN_FDS_START

static inline void
loadLibgit2()
{
#if USE_LIBGIT2
    if (g_args->git() && !osLoadLibgit2())
        g_args->disableGit();
#endif
}

static void
scrubEnvironment()
{
    // Scrub other emulators
    unsetenv("VTE_VERSION");
}

[[noreturn]] static void
runStandalone()
{
    int rc = 1;
    unsigned flavor = Tsq::FlavorStandalone;
    osCreateRuntimeDir(g_args->rundir(), s_udspath);
    loadLibgit2();
    scrubEnvironment();

    try {
        g_listener = new TermListener(STDIN_FILENO, STDOUT_FILENO, flavor);
        rc = g_listener->run(-1);
        delete g_listener;
    }
    catch (const std::exception &e) {
        LOGERR("%s\n", e.what());
    }

    exit(rc == 0 ? EXITCODE_SUCCESS : EXITCODE_SERVERERR);
}

static int
runListener(int initialrd, int initialwd)
{
    int rc = 1;
    unsigned flavor = Tsq::FlavorNormal + g_args->activated();
    osCreatePidFile(s_pidpath);
    loadLibgit2();

    if (g_args->defaultTranslator()->path.empty())
        LOGDBG("Translator: Using compiled-in strings\n");
    else
        LOGDBG("Translator: Loaded %s\n", g_args->defaultTranslator()->path.c_str());

    try {
        g_listener = new TermListener(initialrd, initialwd, flavor);
        rc = g_listener->run(s_listenfd);
        delete g_listener;
    }
    catch (const std::exception &e) {
        LOGERR("%s\n", e.what());
    }

    return (rc == 0) ? EXITCODE_SUCCESS : EXITCODE_SERVERERR;
}

void
runConnect()
{
    if (g_args->activated())
        return;
    if (g_args->standalone())
        runStandalone(); // noreturn

    int fd = osServerConnect(g_args->rundir());
    if (fd == -1) {
        if (g_args->client()) {
            throw ErrnoException("connect", errno);
        } else if (!g_args->listen()) {
            runStandalone(); // noreturn
        }
        // returns
    } else {
        if (!g_args->stdinput())
            throw ErrnoException("connect", EADDRINUSE);

        runForwarder(fd); // noreturn
    }
}

void
runListen()
{
    if (g_args->activated()) {
        osCreateRuntimeDir(SERVER_XDG_DIR, s_pidpath);
        int rc = sd_listen_fds(true);
        if (rc <= 0)
            throw ErrnoException("sd_listen_fds", rc ? -rc : ENOTCONN);
    }
    else {
        osCreateRuntimeDir(g_args->rundir(), s_udspath);
        strcpy(s_pidpath, s_udspath);
        osCreateSocketPath(s_udspath);
        s_listenfd = osLocalListen(s_udspath);
    }
}

int
runServer()
{
    int rc = 0;

    chdir("/");
    scrubEnvironment();

    if (g_args->fork()) {
        int sd[2];
        if (g_args->stdinput()) {
            osSocketPair(sd, true);
        } else {
            sd[0] = -1;
        }

        if (!osFork()) {
            if (g_args->stdinput())
                close(sd[1]);
            osDaemonize();
            rc = runListener(sd[0], sd[0]);
        } else if (g_args->stdinput()) {
            close(s_listenfd);
            close(sd[0]);
            runForwarder(sd[1]); // noreturn
        } else {
            exit(0); // noreturn
        }
    }
    else if (g_args->stdinput()) {
        rc = runListener(STDIN_FILENO, STDOUT_FILENO);
    }
    else {
        rc = runListener(-1, -1);
    }

    if (!g_args->activated()) {
        unlink(s_udspath);
    }
    unlink(s_pidpath);
    return rc;
}
