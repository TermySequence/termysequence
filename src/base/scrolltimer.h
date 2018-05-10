// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QTimer>
#include <QSet>

class TermScrollport;

class TermScrollTimer final: public QTimer
{
    Q_OBJECT

private:
    QSet<TermScrollport*> m_fetches;
    QSet<TermScrollport*> m_searches;
    QSet<TermScrollport*> m_scans;

private slots:
    void timerCallback();
    void handleViewportDestroyed(QObject *object);

public:
    TermScrollTimer(QObject *parent);

    void addViewport(TermScrollport *viewport);
    void setFetchTimer(TermScrollport *viewport);
    void setSearchTimer(TermScrollport *viewport);
    void setScanTimer(TermScrollport *viewport);
    void unsetScanTimer(TermScrollport *viewport);
};
