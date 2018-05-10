// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "thumbframe.h"
#include "thumbwidget.h"

#include <QPainter>
#include <QGuiApplication>
#include <QEvent>

ThumbFrame::ThumbFrame(TermInstance *term, TermManager *manager, QWidget *parent) :
    QWidget(parent),
    m_widget(new ThumbWidget(term, manager, this)),
    m_bounds(1, 1, 0, 0)
{
}

void
ThumbFrame::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    QPen pen(painter.pen());
    pen.setColor(m_color);
    pen.setWidth(0);
    painter.setPen(pen);

    painter.drawRect(m_border);
}

void
ThumbFrame::resizeEvent(QResizeEvent *event)
{
    m_border.setWidth(width() - 1);
    m_border.setHeight(height() - 1);
    m_bounds.setWidth(width() - 2);
    m_bounds.setHeight(height() - 2);
    m_widget->setGeometry(m_bounds);
}

int
ThumbFrame::heightForWidth(int w) const
{
    return 2 + m_widget->heightForWidth(w - 2);
}

int
ThumbFrame::widthForHeight(int h) const
{
    return 2 + m_widget->widthForHeight(h - 2);
}

bool
ThumbFrame::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Polish:
        QWidget::event(event);
        m_color = QGuiApplication::palette().color(QPalette::Active, QPalette::Dark);
        return true;
    case QEvent::LayoutRequest:
        updateGeometry();
        return true;
    default:
        return QWidget::event(event);
    }
}
