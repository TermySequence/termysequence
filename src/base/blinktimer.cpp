// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "blinktimer.h"
#include "viewport.h"
#include "fileviewitem.h"
#include "term.h"
#include "settings/global.h"

TermBlinkTimer::TermBlinkTimer(QObject *parent):
    QTimer(parent),
    m_blink(false),
    m_cursorEnable(g_global->cursorBlink()),
    m_textEnable(g_global->textBlink()),
    m_cursorCount(g_global->cursorBlinks()),
    m_textCount(g_global->textBlinks()),
    m_skipCount(g_global->skipBlinks())
{
    connect(g_global, SIGNAL(cursorBlinkChanged(bool)), SLOT(setCursorEnable(bool)));
    connect(g_global, SIGNAL(textBlinkChanged(bool)), SLOT(setTextEnable(bool)));
    connect(g_global, SIGNAL(cursorBlinksChanged(unsigned)), SLOT(setCursorCount(unsigned)));
    connect(g_global, SIGNAL(textBlinksChanged(unsigned)), SLOT(setTextCount(unsigned)));
    connect(g_global, SIGNAL(skipBlinksChanged(unsigned)), SLOT(setSkipCount(unsigned)));
    connect(g_global, SIGNAL(blinkTimeChanged(unsigned)), SLOT(setBlinkTime(unsigned)));
    setInterval(g_global->blinkTime());

    connect(this, SIGNAL(timeout()), SLOT(timerCallback()));
}

void
TermBlinkTimer::timerCallback()
{
    m_blink = !m_blink;

    bool cursorBlink = m_cursorEnable && m_blink;
    bool textBlink = m_textEnable && m_blink;

    for (auto i = m_stateMap.begin(); i != m_stateMap.end(); ) {
        BlinkBase *target = i.key();
        TermBlinkState &state = i.value();

        if (state.isFinished()) {
            target->updateBlink = false;
            i = m_stateMap.erase(i);
            continue;
        }

        if (state.skipCount > 0) {
            --state.skipCount;
            target->cursorBlink = false;
        } else if (state.cursorCount > 0) {
            target->cursorBlink = (--state.cursorCount > 0) ? cursorBlink : false;
        }

        if (state.textCount > 0) {
            target->textBlink = (--state.textCount > 0) ? textBlink : false;
        }

        ++i;
    }

    if (m_stateMap.isEmpty()) {
        m_blink = false;
        stop();
    }
}

void
TermBlinkTimer::handleInputReceived()
{
    TermViewport *viewport = static_cast<TermViewport*>(sender());
    m_stateMap[viewport].skipCount = (2 * m_skipCount);
    viewport->cursorBlink = false;
}

void
TermBlinkTimer::handleViewportDestroyed(QObject *object)
{
    m_stateMap.remove(static_cast<TermViewport*>(object));
}

void
TermBlinkTimer::handleFileviewDestroyed(QObject *object)
{
    m_stateMap.remove(static_cast<FileNameItem*>(object));
}

void
TermBlinkTimer::addViewport(TermViewport *viewport)
{
    connect(viewport, SIGNAL(destroyed(QObject*)), SLOT(handleViewportDestroyed(QObject*)));
    connect(viewport, SIGNAL(inputReceived()), SLOT(handleInputReceived()));
    viewport->updateBlink = false;
}

void
TermBlinkTimer::addFileview(FileNameItem *item)
{
    connect(item, SIGNAL(destroyed(QObject*)), SLOT(handleFileviewDestroyed(QObject*)));
    item->updateBlink = false;
}

void
TermBlinkTimer::setBlinkEffect(TermViewport *viewport)
{
    TermBlinkState &state = m_stateMap[viewport];
    bool flag = false;

    if (m_cursorEnable && viewport->primary()) {
        state.cursorCount = (2 * m_cursorCount);
        flag = true;
    } else {
        state.cursorCount = state.skipCount = 0;
        viewport->cursorBlink = false;
    }

    if (m_textEnable && viewport->focused() && viewport->term()->blinkSeen()) {
        state.textCount = (2 * m_textCount);
        flag = true;
    } else {
        state.textCount = 0;
        viewport->textBlink = false;
    }

    viewport->updateBlink = flag;

    if (flag && !isActive())
        start();
}

void
TermBlinkTimer::setBlinkEffect(FileNameItem *item)
{
    TermBlinkState &state = m_stateMap[item];
    bool flag = false;

    if (m_textEnable && item->active()) {
        state.textCount = (2 * m_textCount);
        flag = true;
    } else {
        state.textCount = 0;
        item->textBlink = false;
    }

    item->updateBlink = flag;

    if (flag && !isActive())
        start();
}

void
TermBlinkTimer::setCursorEnable(bool cursorEnable)
{
    m_cursorEnable = cursorEnable;
}

void
TermBlinkTimer::setTextEnable(bool textEnable)
{
    m_textEnable = textEnable;
}

void
TermBlinkTimer::setCursorCount(unsigned cursorCount)
{
    m_cursorCount = cursorCount;
}

void
TermBlinkTimer::setTextCount(unsigned textCount)
{
    m_textCount = textCount;
}

void
TermBlinkTimer::setSkipCount(unsigned skipCount)
{
    m_skipCount = skipCount;
}

void
TermBlinkTimer::setBlinkTime(unsigned blinkTime)
{
    setInterval(blinkTime);
}
