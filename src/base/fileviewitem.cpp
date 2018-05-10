// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/color.h"
#include "app/config.h"
#include "fileviewitem.h"
#include "file.h"
#include "filemodel.h"
#include "filetracker.h"
#include "term.h"
#include "listener.h"
#include "lib/grapheme.h"

#include <QPainter>
#include <QtMath>

//
// Regular delegate
//
FileViewItem::FileViewItem(unsigned displayMask, QWidget *parent) :
    QStyledItemDelegate(parent),
    m_displayMask(displayMask)
{
}

void
FileViewItem::paintComplex(QPainter *painter, DisplayCell &dc, Tsq::Unicoding *unicoding) const
{
    qreal totalWidth = dc.rect.width();

    std::string str = dc.text.toStdString();
    for (char &c: str)
        if ((unsigned char)c < 0x20)
            c = ' ';

    const char *cstr = str.c_str();
    Tsq::EmojiWalk ebf(unicoding, str);
    Tsq::CellFlags flags;

    while (ebf.next(flags)) {
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

        totalWidth -= curWidth;
        if (totalWidth <= 0)
            break;

        dc.flags &= ~Tsq::EmojiChar;
        dc.rect.translate(curWidth, 0);
        dc.point.rx() += curWidth;
    }
}

void
FileViewItem::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const TermInstance *term = FILE_TERMP(index);
    QRgb fg = term->fg();
    int fade;

    DisplayCell dc;
    dc.fg = fg;
    dc.text = index.data().toString();
    dc.rect = option.rect;
    dc.point = option.rect.topLeft();
    dc.point.ry() += (option.rect.height() - m_cellSize.height()) / 2 + m_ascent;

    painter->save();
    painter->setFont(m_font);
    painter->setClipRect(option.rect);

    if (option.state & (QStyle::State_Selected|QStyle::State_HasFocus)) {
        painter->fillRect(option.rect, Colors::blend1(term->bg(), fg));
    }
    else if ((fade = index.data(Qt::UserRole).toInt())) {
        painter->fillRect(option.rect, Colors::blend5a(term->bg(), fg, fade));
    }

    if (m_displayMask & 1 << index.column()) {
        paintComplex(painter, dc, term->unicoding());
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

//
// Filename delegate
//
FileNameItem::FileNameItem(QWidget *parent) :
    QStyledItemDelegate(parent),
    BlinkBase{},
    m_arrow(L(" \u2192 ")),
    m_blink(g_listener->blink())
{
}

void
FileNameItem::drawName(QPainter *painter, const QRect &rect, QStyle::State flags,
                       const QModelIndex &index) const
{
    const TermFile *file = FILE_FILEP(index);
    m_displayCells.clear();

    // Compute cells
    DisplayCell dc;
    dc.rect = rect;
    dc.point = rect.topLeft();
    dc.point.ry() += m_ascent;

    if (file->gittify) {
        CellAttributes gattr[2];
        QString gstr[2];
        int ngit = m_term->files()->getGitDisplay(file, gattr, gstr);

        if (ngit) {
            dc.CellAttributes::operator=(gattr[0]);
            dc.text = gstr[0];
            dc.rect.setWidth(m_cellSize.width());
            m_displayCells.emplace_back(dc);
            dc.rect.translate(dc.rect.width(), 0);
            dc.point.rx() += dc.rect.width();

            if (ngit == 2) {
                dc.CellAttributes::operator=(gattr[1]);
                dc.text = gstr[1];
                m_displayCells.emplace_back(dc);
                dc.rect.translate(dc.rect.width(), 0);
                dc.point.rx() += dc.rect.width();
            }

            dc.rect.translate(m_cellSize.width() / 2, 0);
            dc.point.rx() += m_cellSize.width() / 2;
        }
    }

    dc.CellAttributes::operator=(file->fattr);
    dc.text = file->name;
    decomposeStringPixels(dc);

    if (file->classify && file->fileclass) {
        dc.CellAttributes::operator=(CellAttributes());
        dc.text = C(file->fileclass);
        dc.rect.setWidth(m_cellSize.width());
        m_displayCells.emplace_back(dc);
    }

    // Draw
    CellState state(textBlink ? Tsq::Blink|Tsq::Invisible : Tsq::Invisible);
    state.fg = m_term->fg();
    state.bg = m_term->bg();

    painter->setClipRect(rect);
    QRgb blend1;
    int fade;

    if (flags & (QStyle::State_Selected|QStyle::State_HasFocus)) {
        painter->fillRect(rect, blend1 = Colors::blend1(state.bg, state.fg));
    }
    else if ((fade = index.data(Qt::UserRole).toInt())) {
        painter->fillRect(rect, Colors::blend5a(state.bg, state.fg, fade));
    }

    for (DisplayCell &dc: m_displayCells) {
        paintCell(*painter, dc, state);
    }

    if (flags & (QStyle::State_Selected|QStyle::State_HasFocus|QStyle::State_MouseOver)) {
        qreal line = 1 + rect.height() / MARGIN_INCREMENT;
        QRectF brect(rect.x() + (line / 2), rect.y() + (line / 2),
                     rect.width() - line, rect.height() - line);

        QPen pen(painter->pen());
        pen.setColor(state.fg);
        pen.setWidth(line);
        if (flags & (QStyle::State_Selected|QStyle::State_HasFocus))
            pen.setStyle(Qt::DashLine);
        painter->setPen(pen);
        painter->drawRect(brect);
    }

    // Reset font
    if (state.flags & (Tsq::Bold|Tsq::Underline)) {
        QFont font = painter->font();
        font.setWeight(m_fontWeight);
        font.setUnderline(false);
        painter->setFont(font);
    }
}

void
FileNameItem::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const TermFile *file = FILE_FILEP(index);
    m_displayCells.clear();

    // Compute cells
    DisplayCell dc;
    dc.rect = option.rect;
    dc.point = option.rect.topLeft();
    dc.point.ry() += (option.rect.height() - m_cellSize.height()) / 2 + m_ascent;

    if (index.column() == FILE_COLUMN_GIT) {
        if (file->gittify)
        {
            CellAttributes gattr[2];
            QString gstr[2];
            int ngit = m_term->files()->getGitDisplay(file, gattr, gstr);

            if (ngit) {
                dc.CellAttributes::operator=(gattr[0]);
                dc.text = gstr[0];
                dc.rect.setWidth(m_cellSize.width());
                m_displayCells.emplace_back(dc);

                if (ngit == 2) {
                    dc.rect.translate(dc.rect.width(), 0);
                    dc.point.rx() += dc.rect.width();
                    dc.CellAttributes::operator=(gattr[1]);
                    dc.text = gstr[1];
                    m_displayCells.emplace_back(dc);
                }
            }
        }
    }
    else {
        unsigned char clazzch = file->fileclass;

        dc.CellAttributes::operator=(file->fattr);
        dc.text = file->name;
        decomposeStringPixels(dc);

        if (file->islink) {
            dc.CellAttributes::operator=(CellAttributes());
            dc.text = m_arrow;
            dc.rect.setWidth(m_cellSize.width() * m_arrow.size());
            m_displayCells.emplace_back(dc);
            dc.rect.translate(dc.rect.width(), 0);
            dc.point.rx() += dc.rect.width();

            dc.CellAttributes::operator=(file->lattr);
            dc.text = file->link;
            decomposeStringPixels(dc);

            clazzch = file->linkclass;
        }

        if (file->classify && clazzch) {
            dc.CellAttributes::operator=(CellAttributes());
            dc.text = C(file->fileclass);
            dc.rect.setWidth(m_cellSize.width());
            m_displayCells.emplace_back(dc);
        }
    }

    // Draw
    CellState state(textBlink ? Tsq::Blink|Tsq::Invisible : Tsq::Invisible);
    state.fg = m_term->fg();
    state.bg = m_term->bg();

    painter->save();
    painter->setClipRect(option.rect);
    painter->setFont(m_font);

    int fade;
    qreal totalWidth = option.rect.width();

    if (option.state & (QStyle::State_Selected|QStyle::State_HasFocus)) {
        painter->fillRect(option.rect, Colors::blend1(state.bg, state.fg));
    }
    else if ((fade = index.data(Qt::UserRole).toInt())) {
        painter->fillRect(option.rect, Colors::blend5a(state.bg, state.fg, fade));
    }

    for (DisplayCell &dc: m_displayCells) {
        if (totalWidth < dc.rect.width() && !(dc.flags & Tsq::EmojiChar)) {
            dc.text = m_metrics.elidedText(dc.text, Qt::ElideRight, totalWidth);
        }
        paintCell(*painter, dc, state);
        totalWidth -= dc.rect.width();
        if (totalWidth <= 0)
            break;
    }

    if (option.state & QStyle::State_MouseOver) {
        qreal line = 1 + option.rect.height() / MARGIN_INCREMENT;
        QPen pen(painter->pen());
        pen.setColor(state.fg);
        pen.setWidth(line * 2);
        painter->setPen(pen);
        painter->drawRect(option.rect);
    }

    painter->restore();
}

QSize
FileNameItem::sizeHint(const QStyleOptionViewItem &, const QModelIndex &index) const
{
    qreal w;

    if (index.column() == FILE_COLUMN_NAME) {
        const auto *file = (const TermFile *)index.internalPointer();
        w = stringPixelWidth(file->name);

        if (file->classify) {
            w += m_cellSize.width();
        }
        if (file->islink) {
            w += m_arrow.size() * m_cellSize.width();
            w += stringPixelWidth(file->link);
        }
    } else {
        w = 2 * m_cellSize.width();
    }

    return QSize(qCeil(w), qCeil(m_cellSize.height()));
}
