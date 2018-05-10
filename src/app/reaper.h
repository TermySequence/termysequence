// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QThread>
#include <csignal>
#include "os/signal.h"

class ReaperThread final: public QThread
{
    friend void deathHandler(int signal);

private:
    int m_pid;
    static volatile sig_atomic_t s_deathSignal;

    ReaperThread(int pid);
    void run();

public:
    static void launchReaper(int pid);
};
