// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "smartlabel.h"

#include <QResizeEvent>

EllipsizingLabel::EllipsizingLabel(QWidget *parent) :
    QLabel(parent)
{
    setTextFormat(Qt::PlainText);
}

void
EllipsizingLabel::retext(int width)
{
    QFontMetrics metrics = fontMetrics();
    QStringList text;

    for (auto &i: qAsConst(m_lines))
    {
        text.append(metrics.elidedText(i, Qt::ElideRight, width));
    }

    QLabel::setText(text.join('\n'));
}

void
EllipsizingLabel::setText(const QString &text)
{
    m_lines = text.split('\n');
    retext(width());
}

void
EllipsizingLabel::resizeEvent(QResizeEvent *event)
{
    retext(event->size().width());
    QLabel::resizeEvent(event);
}

void
EllipsizingLabel::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::FontChange:
        retext(width());
        // fallthru
    default:
        QLabel::changeEvent(event);
    }
}
