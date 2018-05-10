// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "watchdog.h"
#include "config.h"
#include "v8util.h"

#include <QThread>

WatchdogController *g_watchdog;

//
// Thread worker
//
WatchdogWorker::WatchdogWorker(QThread *thread) :
    m_start(0l),
    m_end(0l),
    m_lastStart(0l),
    m_lastEnd(0l),
    m_attempts(0),
    m_timerId(0),
    m_thread(thread)
{
}

void
WatchdogWorker::timerEvent(QTimerEvent *)
{
    unsigned start = m_start;
    unsigned end = m_end;

    if (start == m_lastStart && end == m_lastEnd) {
        if (++m_attempts == PLUGIN_WATCHDOG_THRESHOLD) {
            if (m_lastStart != m_lastEnd) {
                // Softlock
                i->TerminateExecution();
            } else {
                // Nothing happening
                killTimer(m_timerId);
                m_timerId = 0;
            }
        }
    }
    else {
        m_lastStart = start;
        m_lastEnd = end;
        m_attempts = 0;
    }
}

void
WatchdogWorker::wakeUp()
{
    if (!m_timerId)
        m_timerId = startTimer(PLUGIN_WATCHDOG_INTERVAL);
}

void
WatchdogWorker::quit()
{
    if (m_timerId)
        killTimer(m_timerId);

    m_thread->quit();
}

//
// Thread controller
//
WatchdogController::WatchdogController()
{
    m_thread = new QThread(this);
    m_worker = new WatchdogWorker(m_thread);

    m_worker->moveToThread(m_thread);
    connect(this, &WatchdogController::sigWakeUp, m_worker, &WatchdogWorker::wakeUp);
    connect(this, &WatchdogController::sigQuit, m_worker, &WatchdogWorker::quit);
    m_thread->start();
}

WatchdogController::~WatchdogController()
{
    emit sigQuit();
    m_thread->wait();

    delete m_worker;
}
