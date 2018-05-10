// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "effecttimer.h"
#include "scrollport.h"
#include "manager.h"
#include "settings/global.h"

TermEffectTimer::TermEffectTimer(QObject *parent):
    QTimer(parent),
    m_resizeEnable(g_global->resizeEffect()),
    m_focusEnable(g_global->focusEffect()),
    m_resizeDuration(EFFECT_RESIZE_DURATION),
    m_focusDuration(EFFECT_FOCUS_DURATION)
{
    connect(g_global, SIGNAL(resizeEffectChanged(bool)), SLOT(setResizeEnable(bool)));
    connect(g_global, SIGNAL(focusEffectChanged(int)), SLOT(setFocusEnable(int)));
    setInterval(EFFECT_TIME);

    connect(this, SIGNAL(timeout()), SLOT(timerCallback()));
}

void
TermEffectTimer::timerCallback()
{
    QHash<TermViewport*,TermEffectState>::iterator i;

    for (i = m_stateMap.begin(); i != m_stateMap.end();) {
        TermViewport *viewport = i.key();
        TermEffectState &state = i.value();

        if (state.resizeCount > 0) {
            viewport->setResizeEffect(--state.resizeCount);
        }

        if (state.focusCount > 0) {
            viewport->setFocusEffect(--state.focusCount);
        }

        if (state.isFinished())
            i = m_stateMap.erase(i);
        else
            ++i;
    }

    if (m_stateMap.isEmpty()) {
        stop();
    }
}

void
TermEffectTimer::handleViewportDestroyed(QObject *object)
{
    TermViewport *viewport = static_cast<TermViewport*>(object);

    m_stateMap.remove(viewport);
}

void
TermEffectTimer::addViewport(TermViewport *viewport)
{
    connect(viewport, SIGNAL(destroyed(QObject*)), SLOT(handleViewportDestroyed(QObject*)));
}

void
TermEffectTimer::setResizeEffect(TermViewport *viewport)
{
    TermEffectState &state = m_stateMap[viewport];

    if (m_resizeEnable) {
        viewport->setResizeEffect(state.resizeCount = m_resizeDuration);

        if (!isActive())
            start();
    }
}

void
TermEffectTimer::cancelResizeEffect(TermViewport *viewport)
{
    TermEffectState &state = m_stateMap[viewport];

    if (state.resizeCount)
        viewport->setResizeEffect(state.resizeCount = 0);
}

void
TermEffectTimer::setFocusEffect(TermScrollport *scrollport)
{
    TermEffectState &state = m_stateMap[scrollport];

    if (scrollport->primary() &&
        (m_focusEnable == FocusAlways ||
         (m_focusEnable == FocusSplit && scrollport->manager()->stacks().size() > 1)))
    {
        scrollport->setFocusEffect(state.focusCount = m_focusDuration);

        if (!isActive())
            start();
    }
    else
    {
        scrollport->setFocusEffect(state.focusCount = 0);
    }
}

void
TermEffectTimer::setResizeEnable(bool resizeEnable)
{
    m_resizeEnable = resizeEnable;
}

void
TermEffectTimer::setFocusEnable(int focusEnable)
{
    m_focusEnable = focusEnable;
}

void
TermEffectTimer::setResizeDuration(int resizeDuration)
{
    m_resizeDuration = resizeDuration;
}

void
TermEffectTimer::setFocusDuration(int focusDuration)
{
    m_focusDuration = focusDuration;
}
