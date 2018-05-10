// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QObject>
#include <QAtomicInteger>

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

//
// Thread worker
//
class WatchdogWorker final: public QObject
{
    Q_OBJECT

    friend class WatchdogController;

private:
    QAtomicInteger<unsigned> m_start, m_end;
    unsigned m_lastStart, m_lastEnd;
    int m_attempts;

    int m_timerId;
    QThread *m_thread;

protected:
    void timerEvent(QTimerEvent *event);

public slots:
    void wakeUp();
    void quit();

public:
    WatchdogWorker(QThread *thread);
};

//
// Thread controller
//
class WatchdogController final: public QObject
{
    Q_OBJECT

private:
    QThread *m_thread;
    WatchdogWorker *m_worker;

signals:
    // Internal worker signals
    void sigWakeUp();
    void sigQuit();

public:
    WatchdogController();
    ~WatchdogController();

    inline void begin() { emit sigWakeUp(); }
    inline void enter() { ++m_worker->m_start; }
    inline void leave() { ++m_worker->m_end; }
};

extern WatchdogController *g_watchdog;
