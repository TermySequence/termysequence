// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>
#include <QRectF>

class DisplayIterator;
class TermViewport;

class CursorHighlight final: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal radius READ radius WRITE setRadius)

private:
    const DisplayIterator *m_fontbase;
    const TermViewport *m_viewport;

    qreal m_radius = 0.0;
    qreal m_span;
    qreal m_limit;
    qreal m_scale;

    QRectF m_rings[8];
    QRgb m_colors[8];

protected:
    void paintEvent(QPaintEvent *event);

public:
    CursorHighlight(QWidget *parent, const DisplayIterator *fontbase,
                    const TermViewport *viewport);

    inline qreal radius() const { return m_radius; }
    void setRadius(qreal radius);
};
