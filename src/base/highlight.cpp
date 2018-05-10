// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "highlight.h"
#include "fontbase.h"
#include "viewport.h"
#include "term.h"

#include <QPropertyAnimation>
#include <QPainter>

CursorHighlight::CursorHighlight(QWidget *parent, const DisplayIterator *fontbase,
                                 const TermViewport *viewport) :
    QWidget(parent),
    m_fontbase(fontbase),
    m_viewport(viewport),
    m_span(fontbase->cellSize().width()),
    m_limit(8 * m_span)
{
    for (int i = 0; i < 8; ++i)
        m_colors[i] = viewport->term()->palette().at(i);

    auto *anim = new QPropertyAnimation(this, B("radius"), this);
    anim->setEndValue(2 * m_limit);
    anim->setDuration(CURSOR_HIGHLIGHT_TIME);
    connect(anim, SIGNAL(finished()), SLOT(deleteLater()));
    anim->start();

    m_scale = (qreal)parent->size().width() / (m_viewport->width() * m_span);
    resize(parent->size());
    raise();
    show();
}

void
CursorHighlight::setRadius(qreal radius)
{
    m_radius = radius;
    update();

    QPointF center(m_viewport->cursor());
    QSizeF cellSize = m_fontbase->cellSize();
    center.rx() *= cellSize.width();
    center.rx() += cellSize.width() / 2;
    center.ry() *= cellSize.height();
    center.ry() += cellSize.height() / 2;

    for (int i = 0; i < 8; ++i) {
        if (radius > 0 && radius < m_limit) {
            m_rings[i].setRect(0, 0, 2 * radius, 2 * radius);
            m_rings[i].moveCenter(center);
        } else {
            m_rings[i] = QRectF();
        }

        radius -= m_span;
    }
}

void
CursorHighlight::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.scale(m_scale, m_scale);

    QPen pen;
    pen.setWidthF(m_span);

    for (int i = 0; i < 8; ++i)
        if (!m_rings[i].isNull()) {
            pen.setColor(m_colors[i]);
            painter.setPen(pen);
            painter.drawEllipse(m_rings[i]);
        }
}
