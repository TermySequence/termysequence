// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"

#include <QWidget>
#include <QVector>

QT_BEGIN_NAMESPACE
class QSvgRenderer;
QT_END_NAMESPACE
class TermManager;

class TermPeek final: public QWidget, public FontBase
{
private:
    TermManager *m_manager;
    int m_timerId = 0;

    enum ColorName { NormalBg, NormalFg, ActiveBg, ActiveFg, Dark, NColors };
    QColor m_colors[NColors];

    struct TextCell {
        ColorName fg;
        QString text;
        QPointF point;
    };

    std::vector<std::pair<QRect,ColorName>> m_fills;
    std::vector<TextCell> m_cells;
    std::vector<std::pair<QRectF,QSvgRenderer*>> m_icons;
    QRect m_bounds;
    QPoint m_offset;

    QString m_text1, m_text2;

    void calculateColors();
    void relayout();

protected:
    bool event(QEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void timerEvent(QTimerEvent *event);

public:
    TermPeek(TermManager *manager, QWidget *parent);

    void bringUp();
};
