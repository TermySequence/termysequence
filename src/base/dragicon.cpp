// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "dragicon.h"

#include <QPainter>
#include <QResizeEvent>

static QPainterPath s_acceptPath, s_arrowPath, s_rejectPath;

DragIcon::DragIcon(QWidget *parent) :
    QWidget(parent)
{
}

void
DragIcon::setAccepted(bool accepted)
{
    if (m_accepted != accepted) {
        m_accepted = accepted;
        update();
    }
}

void
DragIcon::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(m_pos);
    painter.scale(m_scale, m_scale);

    if (m_accepted) {
        painter.fillPath(s_acceptPath, QColor(0, 0x74, 0));
        painter.fillPath(s_arrowPath, Qt::white);
    } else {
        painter.fillPath(s_rejectPath, Qt::red);
    }
}

void
DragIcon::resizeEvent(QResizeEvent *event)
{
    int w = event->size().width();
    int h = event->size().height();

    if (h < w * 2) {
        m_pos.setX(w / 2 - h / 4);
        m_pos.setY(0);
        m_scale = (qreal)h / 400.0;
    } else {
        m_pos.setX(0);
        m_pos.setY(h - 2 * w);
        m_scale = (qreal)w / 200.0;
    }
}

void
DragIcon::initialize()
{
    // Accept
    s_acceptPath.addEllipse(66.0, 0.0, 66.0, 66.0);
    s_acceptPath.addEllipse(66.0, 100.0, 66.0, 66.0);
    s_acceptPath.addEllipse(0.0, 200.0, 200.0, 200.0);

    // Accept Arrow
    s_arrowPath.addRect(75.0, 250.0, 50.0, 50.0);
    s_arrowPath.moveTo(40.0, 300.0);
    s_arrowPath.lineTo(160.0, 300.0);
    s_arrowPath.lineTo(100.0, 360.0);
    s_arrowPath.closeSubpath();

    // Reject
    s_rejectPath.addEllipse(66.0, 0.0, 66.0, 66.0);
    s_rejectPath.addEllipse(66.0, 100.0, 66.0, 66.0);
    s_rejectPath.addEllipse(0.0, 200.0, 200.0, 200.0);

    QPainterPath inner;
    inner.addEllipse(40.0, 240.0, 120.0, 120.0);
    s_rejectPath -= inner;

    QVector<QPointF> barPoly = { QPointF(30.0, 248.0),
                                 QPointF(48.0, 230.0),
                                 QPointF(170.0, 352.0),
                                 QPointF(152.0, 370.0) };
    QPainterPath bar;
    bar.addPolygon(barPoly);
    bar.closeSubpath();
    s_rejectPath += bar;
}
