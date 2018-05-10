// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

class ThumbWidget;
class TermInstance;
class TermManager;

class ThumbFrame final: public QWidget
{
private:
    ThumbWidget *m_widget;
    QRect m_bounds;
    QRect m_border;
    QColor m_color;

protected:
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

public:
    ThumbFrame(TermInstance *term, TermManager *manager, QWidget *parent);

    int heightForWidth(int w) const;
    int widthForHeight(int h) const;
};
