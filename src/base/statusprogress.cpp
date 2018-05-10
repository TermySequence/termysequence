// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/color.h"
#include "statusprogress.h"

#include <QPainter>
#include <QMouseEvent>

void
StatusProgress::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
        event->accept();
    }
}

void
StatusProgress::enterEvent(QEvent *)
{
    m_hover = true;
    update();
}

void
StatusProgress::leaveEvent(QEvent *)
{
    m_hover = false;
    update();
}

void
StatusProgress::paintEvent(QPaintEvent *event)
{
    if (m_hover) {
        QPainter painter(this);
        painter.fillRect(rect(), m_bg);
    }

    QProgressBar::paintEvent(event);
}

bool
StatusProgress::event(QEvent *event)
{
    bool rc = QProgressBar::event(event);

    if (event->type() == QEvent::Polish) {
        m_bg = Colors::whiten(palette().color(QPalette::Highlight));
        m_bg.setAlphaF(0.4);
    }

    return rc;
}
