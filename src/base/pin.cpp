// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "pin.h"
#include "term.h"
#include "scrollport.h"
#include "buffers.h"
#include "region.h"
#include "manager.h"
#include "mainwindow.h"
#include "minimap.h"
#include "mark.h"

#include <QPainter>
#include <QSvgRenderer>
#include <QMenu>
#include <QContextMenuEvent>

#define TR_TEXT2 TL("window-text", "Location of a recent prompt")
#define TR_TEXT4 TL("window-text", "Location of selected prompt")
#define TR_TEXT5 TL("window-text", "Selected text: %1 lines")
#define TR_TEXT6 TL("window-text", "Location of current search match")
#define TR_TEXT7 TL("window-text", "Row") + A(": ")

TermMinipin::TermMinipin(TermScrollport *scrollport, QWidget *parent) :
    QWidget(parent),
    m_term(scrollport->term()),
    m_scrollport(scrollport)
{
    setCursor(Qt::PointingHandCursor);
}

bool
TermMinipin::setRegion(const Region *region)
{
    int bgi, fgi;
    QString tooltip;
    const Region *job;

    m_regionId = region->id();
    m_regionType = region->type();
    m_startRow = region->startRow;
    m_endRow = m_startRow + 1;
    m_renderer = nullptr;

    switch (m_regionType) {
    case Tsqt::RegionUser:
        m_endRow = region->trueEndRow();
        TermMark::getNoteColors(bgi, fgi);
        tooltip = TermMark::getNoteTooltip(region, m_term);
        m_text = region->attributes.value(g_attr_REGION_NOTECHAR);
        break;
    case Tsqt::RegionJob:
        tooltip = TR_TEXT2;
        m_renderer = region->renderer;
        if (TermMark::getJobMark(region, tooltip, bgi, fgi).isEmpty())
            return false;
        break;
    case Tsqt::RegionPrompt:
        tooltip = TR_TEXT4;
        job = m_term->buffers()->safeRegion(region->parent);
        m_renderer = job ? job->renderer : nullptr;
        TermMark::getJobMark(job, tooltip, bgi, fgi);
        bgi = PALETTE_SH_MARK_SELECTED_BG;
        fgi = PALETTE_SH_MARK_SELECTED_FG;
        break;
    case Tsqt::RegionSelection:
        m_endRow = region->trueEndRow();
        bgi = PALETTE_SH_MARK_MATCH_BG;
        fgi = PALETTE_SH_MARK_MATCH_FG;
        tooltip = TR_TEXT5.arg(m_endRow - m_startRow);
        break;
    default:
        bgi = PALETTE_SH_MARK_MATCH_BG;
        fgi = PALETTE_SH_MARK_MATCH_FG;
        tooltip = TR_TEXT6;
        break;
    }

    setToolTip(TR_TEXT7 + QString::number(m_startRow) + '\n' + tooltip);

    const TermPalette &palette = m_term->palette();
    QRgb bg = palette.at(bgi), fg = palette.at(fgi);

    if (PALETTE_IS_DISABLED(bg)) {
        m_bg = PALETTE_IS_DISABLED(fg) ? m_term->fg() : fg;
        m_fg = m_term->bg();
    } else {
        m_bg = bg;
        m_fg = PALETTE_IS_DISABLED(fg) ? m_term->fg() : fg;
    }

    update();
    return true;
}

void
TermMinipin::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_bg);

    QPen pen(painter.pen());
    pen.setWidth(0);
    pen.setColor(m_fg);

    if (m_renderer)
    {
        m_renderer->render(&painter, m_icon);
    }
    else if (m_regionType == Tsqt::RegionSelection)
    {
        QSizeF cellSize = static_cast<TermMinimap*>(parentWidget())->cellSize();
        int h = cellSize.height() / 2.0;
        QRect r(0, 0, width() / 2, h / 2);

        painter.fillRect(rect(), Qt::black);

        while (r.y() < height() - 1) {
            painter.fillRect(r, Qt::white);
            r.translate(0, h);
        }

        r.moveTo(width() / 2, h / 2);

        while (r.y() < height() - 1) {
            painter.fillRect(r, Qt::white);
            r.translate(0, h);
        }

        pen.setColor(Qt::gray);
    }
    else if (m_regionType == Tsqt::RegionUser)
    {
        painter.setPen(pen);
        painter.setFont(m_term->font());
        painter.drawText(rect(), Qt::AlignCenter, m_text);
    }

    painter.setPen(pen);
    painter.drawRect(m_border);
}

void
TermMinipin::resizeEvent(QResizeEvent *event)
{
    int w = width();
    int h = height();

    m_border.setWidth(w - 1);
    m_border.setHeight(h - 1);

    if (w > h) {
        m_icon.setRect(0, 0, h, h);
    } else {
        m_icon.setRect(0, (h - w) / 2, w, w);
    }
}

void
TermMinipin::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *m = nullptr;
    const Region *r;

    switch (m_regionType) {
    case Tsqt::RegionPrompt:
        if ((r = m_term->buffers()->safeRegion(m_regionId)))
            m = m_scrollport->manager()->parent()->getJobPopup(m_term, r->parent, this);
        break;
    case Tsqt::RegionJob:
        m = m_scrollport->manager()->parent()->getJobPopup(m_term, m_regionId, this);
        break;
    case Tsqt::RegionUser:
        m = m_scrollport->manager()->parent()->getNotePopup(m_term, m_regionId, this);
        break;
    default:
        break;
    }

    if (m) {
        m->popup(event->globalPos());
        event->accept();
    } else {
        event->ignore();
    }
}

void
TermMinipin::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        switch (m_regionType) {
        case Tsqt::RegionSelection:
        case Tsqt::RegionSearch:
            m_scrollport->scrollToRow(m_startRow, true);
            break;
        default:
            m_scrollport->scrollToRegion(m_regionId, true, false);
            break;
        }

        event->accept();
    }
}
