// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "scrolltimer.h"
#include "scrollport.h"

TermScrollTimer::TermScrollTimer(QObject *parent):
    QTimer(parent)
{
    connect(this, SIGNAL(timeout()), SLOT(timerCallback()));
}

void
TermScrollTimer::timerCallback()
{
    auto i = m_fetches.begin(), j = m_searches.begin(), k = m_scans.begin();

    if (i != m_fetches.end()) {
        (*i)->fetchCallback();
        m_fetches.erase(i);
    }
    if (j != m_searches.end()) {
        (*j)->searchCallback();
        m_searches.erase(j);
    }

    while (k != m_scans.end()) {
        if ((*k)->scanCallback())
            ++k;
        else
            k = m_scans.erase(k);
    }

    if (m_fetches.isEmpty() && m_searches.isEmpty() && m_scans.isEmpty())
        stop();
}

void
TermScrollTimer::handleViewportDestroyed(QObject *object)
{
    TermScrollport *viewport = static_cast<TermScrollport*>(object);

    m_fetches.remove(viewport);
    m_searches.remove(viewport);
}

void
TermScrollTimer::addViewport(TermScrollport *viewport)
{
    connect(viewport, SIGNAL(destroyed(QObject*)), SLOT(handleViewportDestroyed(QObject*)));
}

void
TermScrollTimer::setFetchTimer(TermScrollport *viewport)
{
    m_fetches.insert(viewport);

    if (!isActive())
        start();
}

void
TermScrollTimer::setSearchTimer(TermScrollport *viewport)
{
    m_searches.insert(viewport);

    if (!isActive())
        start();
}

void
TermScrollTimer::setScanTimer(TermScrollport *viewport)
{
    m_scans.insert(viewport);

    if (!isActive())
        start();
}

void
TermScrollTimer::unsetScanTimer(TermScrollport *viewport)
{
    m_scans.remove(viewport);
}
