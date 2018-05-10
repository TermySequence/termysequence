// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/color.h"
#include "app/config.h"
#include "jobviewitem.h"
#include "jobmodel.h"
#include "lib/grapheme.h"

#include <QPainter>
#include <QSvgRenderer>

JobViewItem::JobViewItem(unsigned displayMask, QWidget *parent) :
    QStyledItemDelegate(parent),
    m_displayMask(displayMask)
{
}

void
JobViewItem::paintComplex(QPainter *painter, DisplayCell &dc, const QModelIndex &index) const
{
    qreal totalWidth = dc.rect.width();

    auto *renderer = JOB_ICONP(index);
    if (renderer) {
        qreal off = m_cellSize.height() - 2;
        QRectF rect(dc.rect.x(),
                    dc.rect.y() + (dc.rect.height() - m_cellSize.height()) / 2 + 1,
                    off, off);

        renderer->render(painter, rect);

        off = off * 5 / 4;
        dc.rect.translate(off, 0);
        dc.point.rx() += off;
        totalWidth -= off;
    }

    std::string str = dc.text.toStdString();
    for (char &c: str)
        if ((unsigned char)c < 0x20)
            c = ' ';

    const char *cstr = str.c_str();
    Tsq::EmojiWalk ebf(JOB_CODINGP(index), str);
    Tsq::CellFlags flags;

    while (totalWidth > 0 && ebf.next(flags)) {
        qreal curWidth;

        if (flags) {
            dc.text = QString::fromStdString(ebf.getEmojiName());
            curWidth = m_cellSize.width() * 2;
            dc.flags |= Tsq::EmojiChar;
        } else {
            dc.text = QString::fromUtf8(cstr + ebf.start(), ebf.end() - ebf.start());
            curWidth = m_metrics.width(dc.text);
            if (curWidth > totalWidth)
                dc.text = m_metrics.elidedText(dc.text, Qt::ElideRight, totalWidth);
        }

        dc.rect.setWidth(curWidth);
        paintSimple(*painter, dc);
        dc.flags &= ~Tsq::EmojiChar;

        dc.rect.translate(curWidth, 0);
        dc.point.rx() += curWidth;
        totalWidth -= curWidth;
    }
}

void
JobViewItem::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRgb bg = index.data(Qt::BackgroundRole).toUInt();
    QRgb fg = index.data(Qt::ForegroundRole).toUInt();
    bool sel = option.state & (QStyle::State_Selected|QStyle::State_HasFocus);

    if (index.column()) {
        int fade = index.data(Qt::UserRole).toInt();
        if (sel)
            bg = Colors::blend1(bg, fg);
        else if (fade)
            bg = Colors::blend5a(bg, fg, fade);
    }

    DisplayCell dc;
    dc.flags = sel ? Tsq::Bold : 0;
    dc.fg = fg;
    dc.text = index.data().toString();
    dc.rect = option.rect;
    dc.point = option.rect.topLeft();
    dc.point.ry() += (option.rect.height() - m_cellSize.height()) / 2 + m_ascent;

    painter->save();
    painter->setFont(m_font);
    painter->setClipRect(option.rect);
    painter->fillRect(option.rect, bg);

    if (m_displayMask & 1 << index.column()) {
        paintComplex(painter, dc, index);
    } else {
        dc.text = m_metrics.elidedText(dc.text, Qt::ElideRight, option.rect.width());
        paintSimple(*painter, dc);
    }

    if (option.state & QStyle::State_MouseOver) {
        qreal line = 1 + option.rect.height() / MARGIN_INCREMENT;
        QPen pen(painter->pen());
        pen.setWidth(line * 2);
        painter->setPen(pen);
        painter->drawRect(option.rect);
    }

    painter->restore();
}
