// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QTimer>
#include <QHash>

class TermViewport;
class TermScrollport;

struct TermEffectState
{
    int resizeCount;
    int focusCount;

    TermEffectState(): resizeCount(0), focusCount(0) {}

    inline bool isFinished() { return (resizeCount == 0) && (focusCount == 0); }
};

class TermEffectTimer final: public QTimer
{
    Q_OBJECT

private:
    QHash<TermViewport*,TermEffectState> m_stateMap;

    bool m_resizeEnable;
    int m_focusEnable;

    int m_resizeDuration;
    int m_focusDuration;

private slots:
    void timerCallback();
    void handleViewportDestroyed(QObject *object);

    void setResizeEnable(bool resizeEnable);
    void setFocusEnable(int focusEnable);
    void setResizeDuration(int resizeDuration);
    void setFocusDuration(int focusDuration);

public:
    TermEffectTimer(QObject *parent);

    void addViewport(TermViewport *viewport);

    void setResizeEffect(TermViewport *viewport);
    void cancelResizeEffect(TermViewport *viewport);
    void setFocusEffect(TermScrollport *scrollport);
};
