// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "colorpreview.h"

#include <QtMath>
#include <QEvent>

ColorPreview::ColorPreview()
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

void
ColorPreview::setColor(const QColor &color)
{
    m_color = color;
    update();
}

bool
ColorPreview::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Polish:
        QWidget::event(event);
        calculateCellSize(font());
        return true;
    default:
        return QWidget::event(event);
    }
}

void
ColorPreview::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_color);
}

QSize
ColorPreview::sizeHint() const
{
    ensurePolished();
    int w = qCeil(12 * m_cellSize.width());
    int h = qCeil(2 * m_cellSize.height());
    return QSize(w, h);
}
