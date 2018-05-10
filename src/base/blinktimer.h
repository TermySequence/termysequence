// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QTimer>
#include <QHash>

class TermViewport;
class FileNameItem;

struct BlinkBase
{
    bool cursorBlink;
    bool textBlink;
    bool updateBlink;
};

struct TermBlinkState
{
    unsigned cursorCount;
    unsigned textCount;
    unsigned skipCount;

    TermBlinkState(): cursorCount(0), textCount(0), skipCount(0) {}

    inline bool isFinished() { return (cursorCount == 0) && (textCount == 0); }
};

class TermBlinkTimer final: public QTimer
{
    Q_OBJECT

private:
    QHash<BlinkBase*,TermBlinkState> m_stateMap;

    bool m_blink;
    bool m_cursorEnable;
    bool m_textEnable;

    unsigned m_cursorCount;
    unsigned m_textCount;
    unsigned m_skipCount;

private slots:
    void timerCallback();
    void handleInputReceived();
    void handleViewportDestroyed(QObject *object);
    void handleFileviewDestroyed(QObject *object);

    void setCursorEnable(bool cursorEnable);
    void setTextEnable(bool textEnable);
    void setCursorCount(unsigned cursorCount);
    void setTextCount(unsigned textCount);
    void setSkipCount(unsigned skipCount);
    void setBlinkTime(unsigned blinkTime);

public:
    TermBlinkTimer(QObject *parent);

    void addViewport(TermViewport *viewport);
    void setBlinkEffect(TermViewport *viewport);

    void addFileview(FileNameItem *item);
    void setBlinkEffect(FileNameItem *item);
};
