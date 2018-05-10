// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "signal.h"

#include <csignal>

void
osSetupServerSignalHandlers()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));

    act.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &act, NULL);

    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL);

    act.sa_handler = deathHandler;
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGTTIN, &act, NULL);
    sigaction(SIGTTOU, &act, NULL);

    act.sa_handler = reloadHandler;
    sigaction(SIGUSR1, &act, NULL);
}

void
osSetupClientSignalHandlers()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));

    act.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &act, NULL);

    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL);

    act.sa_handler = deathHandler;
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}

void
osIgnoreBackgroundSignals()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    sigaction(SIGTTOU, &act, NULL);
}

void
osKillProcess(int pgrp, int signal)
{
    killpg(pgrp, signal ? signal : SIGTERM);
}
