// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"

#include <QObject>
#include <QSet>

class TermListener;
class TermInstance;
class TermBuffers;

class TermFetchTimer final: public QObject
{
    Q_OBJECT

private:
    QSet<TermBuffers*> m_staged;
    QSet<TermBuffers*> m_active;
    QSet<TermBuffers*>::iterator m_iter;

    int m_atimerId = 0, m_stimerId = 0;
    int m_atimerInterval = 0, m_stimerInterval;

    bool m_enabled;
    int m_idleThreshold;

    void disableTerm(TermBuffers *buffers);
    void stageTerm(TermBuffers *buffers);
    void activateTerm(TermBuffers *buffers);

    bool activeTimerHelper(int64_t now);

private slots:
    void handleTermAdded(TermInstance *term);
    void handleTermRemoved(TermInstance *term);

    void handleRowFetched();
    void handleBufferChanged();
    void handleBufferReset();

    void handleThrottleChanged(bool throttled);
    void handleRatelimitChanged(bool ratelimited);

    void handleSpeedChanged(int speed);

protected:
    void timerEvent(QTimerEvent *event);

public:
    TermFetchTimer(TermListener *parent);
};
