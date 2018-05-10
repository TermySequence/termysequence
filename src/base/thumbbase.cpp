// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/color.h"
#include "app/config.h"
#include "thumbbase.h"
#include "idbase.h"
#include "barwidget.h"
#include "dragicon.h"

#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>

#define CARET_BIG (CARET_WIDTH + 0.5)
#define CARET_SMALL (CARET_WIDTH - 0.5)

ThumbBase::ThumbBase(IdBase *target, TermManager *manager, BarWidget *parent):
    QWidget(parent),
    m_manager(manager),
    m_parent(parent),
    m_targetIdStr(target->idStr())
{
}

void
ThumbBase::calculateColors(bool lightenActive)
{
    // Load colors
    const QPalette &pal = palette();
    m_colors[NormalFg] = pal.color(QPalette::Active, QPalette::WindowText);

    m_colors[ActiveBg] = pal.color(QPalette::Active, QPalette::Highlight);
    m_colors[ActiveFg] = pal.color(QPalette::Active, QPalette::HighlightedText);

    m_colors[InactiveBg] = pal.color(QPalette::Inactive, QPalette::Highlight);
    if (lightenActive || m_colors[ActiveBg] == m_colors[InactiveBg])
        m_colors[lightenActive ? ActiveBg : InactiveBg] = Colors::whiten(m_colors[ActiveBg]);

    m_colors[ActiveHoverBg] = m_colors[ActiveBg].lighter(110);
    m_colors[InactiveHoverBg] = m_colors[InactiveBg].lighter(110);
    m_colors[InactiveFg] = pal.color(QPalette::Inactive, QPalette::HighlightedText);

    m_colors[HoverBg] = m_colors[InactiveBg];
    m_colors[HoverBg].setAlphaF(0.4);
}

void
ThumbBase::paintTop(QPainter *painter)
{
    QBrush fg(m_colors[m_fgName]);
    QPainterPath path;
    qreal w = width();

    painter->setPen(fg.color());
    painter->drawLine(0.0, 0.0, width(), 0.0);

    painter->setPen(QPen(fg, 1.0));
    painter->setBrush(fg);
    painter->setRenderHint(QPainter::Antialiasing, true);

    path.moveTo(0.5, 0.5);
    path.lineTo(CARET_BIG, 0.5);
    path.lineTo(0.5, CARET_BIG);
    path.lineTo(0.5, 0.5);
    painter->drawPath(path);

    path.moveTo(w - 0.5, 0.5);
    path.lineTo(w - 0.5, CARET_BIG);
    path.lineTo(w - CARET_BIG, 0.5);
    path.lineTo(w - 0.5, 0.5);
    painter->drawPath(path);
}

void
ThumbBase::paintBottom(QPainter *painter)
{
    QBrush fg(m_colors[m_fgName]);
    QPainterPath path;
    qreal w = width(), h = height();

    // painter->setPen(fg.color());
    // painter->drawLine(0.0, height() - 1, width(), height() - 1);

    painter->setPen(QPen(fg, 1.0));
    painter->setBrush(fg);
    painter->setRenderHint(QPainter::Antialiasing, true);

    path.moveTo(0.5, h - 0.5);
    path.lineTo(0.5, h - CARET_SMALL);
    path.lineTo(CARET_SMALL, h - 0.5);
    path.lineTo(0.5, h - 0.5);
    painter->drawPath(path);

    path.moveTo(w - 0.5, h - 0.5);
    path.lineTo(w - CARET_SMALL, h - 0.5);
    path.lineTo(w - 0.5, h - CARET_SMALL);
    path.lineTo(w - 0.5, h - 0.5);
    painter->drawPath(path);
}

void
ThumbBase::paintLeft(QPainter *painter)
{
    QBrush fg(m_colors[m_fgName]);
    QPainterPath path;
    qreal h = height();

    painter->setPen(fg.color());
    painter->drawLine(0.0, 0.0, 0.0, height());

    painter->setPen(QPen(fg, 1.0));
    painter->setBrush(fg);
    painter->setRenderHint(QPainter::Antialiasing, true);

    path.moveTo(0.5, 0.5);
    path.lineTo(CARET_BIG, 0.5);
    path.lineTo(0.5, CARET_BIG);
    path.lineTo(0.5, 0.5);
    painter->drawPath(path);

    path.moveTo(0.5, h - 0.5);
    path.lineTo(0.5, h - CARET_BIG);
    path.lineTo(CARET_BIG, h - 0.5);
    path.lineTo(0.5, h - 0.5);
    painter->drawPath(path);
}

void
ThumbBase::paintRight(QPainter *painter)
{
    QBrush fg(m_colors[m_fgName]);
    QPainterPath path;
    qreal w = width(), h = height();

    // painter->setPen(fg.color());
    // painter->drawLine(width() - 1, 0.0, width() - 1, height());

    painter->setPen(QPen(fg, 1.0));
    painter->setBrush(fg);
    painter->setRenderHint(QPainter::Antialiasing, true);

    path.moveTo(w - 0.5, 0.5);
    path.lineTo(w - 0.5, CARET_SMALL);
    path.lineTo(w - CARET_SMALL, 0.5);
    path.lineTo(w - 0.5, 0.5);
    painter->drawPath(path);

    path.moveTo(w - 0.5, h - 0.5);
    path.lineTo(w - CARET_SMALL, h - 0.5);
    path.lineTo(w - 0.5, h - CARET_SMALL);
    path.lineTo(w - 0.5, h - 0.5);
    painter->drawPath(path);
}

void
ThumbBase::paintCommon(QPainter *painter)
{
    if (m_reorderingBefore) {
        if (m_parent->horizontal())
            paintLeft(painter);
        else
            paintTop(painter);
    }
    else if (m_reorderingAfter) {
        if (m_parent->horizontal())
            paintRight(painter);
        else
            paintBottom(painter);
    }
}

void
ThumbBase::resizeEvent(QResizeEvent *)
{
    int h = height();
    m_dragBounds.setRect(0, h / 4, width(), h / 2);
}

void
ThumbBase::clearDrag()
{
    m_reorderingBefore = m_reorderingAfter = m_normalDrag = false;

    if (m_drag)
        m_drag->hide();
}

bool
ThumbBase::setNormalDrag(const QMimeData *)
{
    // Subclass must set m_normalDrag to true
    // Subclass must m_acceptDrag as appropriate

    if (!m_drag)
        m_drag = new DragIcon(this);

    m_drag->setAccepted(m_acceptDrag);
    m_drag->setGeometry(m_dragBounds);
    m_drag->raise();
    m_drag->show();

    return m_acceptDrag;
}

void
ThumbBase::setReorderDrag(bool before)
{
    m_reorderingBefore = before;
    m_reorderingAfter = !before;
    update();
}

void
ThumbBase::mousePressEvent(QMouseEvent *event)
{
    m_dragStartPosition = event->pos();
    m_clicking = true;
    mouseAction(-1);
    event->accept();
}

void
ThumbBase::mouseMoveEvent(QMouseEvent *event)
{
    int dist = (event->pos() - m_dragStartPosition).manhattanLength();
    bool isdrag = dist >= QApplication::startDragDistance();

    if (isdrag) {
        m_clicking = false;
        if (event->buttons() & Qt::LeftButton) {
            QMimeData *data = new QMimeData;
            data->setData(REORDER_MIME_TYPE, m_targetIdStr.toLatin1());
            data->setText(m_targetIdStr);

            QDrag *drag = new QDrag(this);
            drag->setMimeData(data);
            drag->exec(Qt::CopyAction);
        }
    }
    event->accept();
}

void
ThumbBase::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clicking) {
        m_clicking = false;
        switch (event->button()) {
        case Qt::LeftButton:
            if (event->modifiers() == Qt::ControlModifier)
                mouseAction(1);
            else if (event->modifiers() == Qt::ShiftModifier)
                mouseAction(2);
            break;
        case Qt::MiddleButton:
            if (event->modifiers() == 0)
                mouseAction(3);
            break;
        default:
            break;
        }
    }
    event->accept();
}

void
ThumbBase::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->modifiers() == 0)
        mouseAction(0);

    event->accept();
}

bool
ThumbBase::hidden() const
{
    return false;
}
