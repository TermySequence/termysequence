// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QProgressBar>

class StatusProgress final: public QProgressBar
{
    Q_OBJECT

private:
    bool m_hover = false;
    QColor m_bg;

protected:
    void mousePressEvent(QMouseEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void paintEvent(QPaintEvent *event);
    bool event(QEvent *event);

signals:
    void clicked();
};
