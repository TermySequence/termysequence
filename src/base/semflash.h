// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"

#include <QObject>

class TermBuffers;

class SemanticFlash final: public QObject
{
    Q_OBJECT

private:
    TermBuffers *m_buffers;
    regionid_t m_id;
    int m_timeout;

protected:
    void timerEvent(QTimerEvent *event);

signals:
    void startFlash(unsigned duration);

public:
    SemanticFlash(TermBuffers *buffers, regionid_t id);
    void start(unsigned duration);

    inline int timeout() const { return m_timeout; }
};
