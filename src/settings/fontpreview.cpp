// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "fontpreview.h"

#include <QtMath>
#include <QEvent>
#include <QPainter>

FontPreview::FontPreview()
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
}

void
FontPreview::setFont(const QFont &font)
{
    m_font = font;
    m_name = font.family();

    calculateCellSize(font);

    int w = qCeil(m_name.size() * m_cellSize.width());
    int h = qCeil(2 * m_cellSize.height());

    if (m_sizeHint.width() < w)
        m_sizeHint.setWidth(w);
    if (m_sizeHint.height() < h)
        m_sizeHint.setHeight(h);

    updateGeometry();
    update();
}

void
FontPreview::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setFont(m_font);
    painter.drawText(rect(), Qt::AlignCenter, m_name);
}

QSize
FontPreview::sizeHint() const
{
    return m_sizeHint;
}
