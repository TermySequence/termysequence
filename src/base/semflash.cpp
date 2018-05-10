// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "semflash.h"
#include "buffers.h"

SemanticFlash::SemanticFlash(TermBuffers *buffers, regionid_t id) :
    QObject(buffers),
    m_buffers(buffers),
    m_id(id)
{
}

void
SemanticFlash::start(unsigned duration)
{
    emit startFlash(duration);
    startTimer(m_timeout = duration * BLINK_TIME);
    m_timeout /= 100;
}

void
SemanticFlash::timerEvent(QTimerEvent *)
{
    m_buffers->buffer0()->deleteSemantic(m_id);
    deleteLater();
}
