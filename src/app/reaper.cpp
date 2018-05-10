// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "reaper.h"
#include "os/wait.h"

volatile sig_atomic_t ReaperThread::s_deathSignal;

ReaperThread::ReaperThread(int pid) :
    m_pid(pid)
{
}

void
ReaperThread::run()
{
    while (!s_deathSignal && !osWaitForChild(m_pid));
}

void
ReaperThread::launchReaper(int pid)
{
    auto *worker = new ReaperThread(pid);
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}
