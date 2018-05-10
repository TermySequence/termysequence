// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QScrollBar>

class TermScrollport;
class TermBuffers;

class TermScroll final: public QScrollBar
{
    Q_OBJECT

private:
    TermScrollport *m_scrollport;
    TermBuffers *m_buffers;

    int scrollTo(int pos);

private slots:
    void updateScroll();
    void handleScrollRequest(int type);
    void handleInputReceived();
    void handleActionTriggered();

public:
    TermScroll(TermScrollport *scrollport, QWidget *parent);

    void forwardWheelEvent(QWheelEvent *event);
};
