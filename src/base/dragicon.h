// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>
#include <QPoint>

class DragIcon final: public QWidget
{
private:
    QPoint m_pos;
    qreal m_scale;
    bool m_accepted;

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    DragIcon(QWidget *parent);

    void setAccepted(bool accepted);

    static void initialize();
};
