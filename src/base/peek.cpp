// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "peek.h"
#include "manager.h"
#include "server.h"
#include "term.h"
#include "thumbicon.h"
#include "settings/global.h"

#include <QPainter>
#include <QSvgRenderer>

#define NITEMS PEEK_MAX

#define TR_TEXT1 TL("window-text", "%1 more") + A("...")
#define TR_TEXT2 TL("window-text", "%1 hidden")

TermPeek::TermPeek(TermManager *manager, QWidget *parent) :
    QWidget(parent),
    m_manager(manager)
{
    m_text1 = TR_TEXT1;
    m_text2 = A("(+") + TR_TEXT2 + ')';

    setVisible(false);
}

void
TermPeek::relayout()
{
    m_fills.clear();
    m_cells.clear();
    m_icons.clear();

    //
    // Server layout
    //
    int sIdx, tIdx, gIdx;
    int truncLeft = 0, truncRight = 0;
    ServerInstance *server = m_manager->activeServer();
    m_manager->findServer(server, sIdx, tIdx, gIdx);
    const auto &servers = m_manager->servers();

    TextCell tc;
    qreal ch = m_cellSize.height();
    qreal coct = ch / 8, ascent = m_ascent + coct;
    qreal lh = 5 * ch / 4, lhalf = lh / 2, lquad = lh / 4;
    qreal sw = 0, sy = 0;

    if (servers.size() > NITEMS) {
        // Server truncation needed
        int onLeft = sIdx;
        int onRight = servers.size() - sIdx - 1;
        sy = lh;

        if (onLeft < onRight) {
            if (onLeft > NITEMS / 2) {
                truncLeft = onLeft - NITEMS / 2;
            }
            truncRight = servers.size() - truncLeft - NITEMS;
        } else {
            truncLeft = onLeft - NITEMS / 2;
            if (onRight > NITEMS / 2) {
                truncRight = servers.size() - truncLeft - NITEMS;
            } else {
                truncLeft -= NITEMS / 2 - onRight;
            }
        }
    }

    if (truncLeft) {
        tc.fg = NormalFg;
        tc.text = L("%1 more...").arg(truncLeft);
        tc.point = QPointF(0, ascent);
        sw = m_metrics.width(tc.text) + lhalf;
        m_cells.emplace_back(std::move(tc));
    }
    for (int i = truncLeft; i < servers.size() && i - truncLeft < NITEMS; ++i) {
        qreal w = ch + lquad;
        // Icon
        m_icons.emplace_back(
            std::make_pair(QRect(0, sy + coct, ch, ch),
                           servers[i]->iconRenderer()));
        // Indicator
        if (servers[i]->indicatorTypes()[0] != ThumbIcon::InvalidType) {
            m_icons.emplace_back(
                std::make_pair(QRect(w, sy + coct, ch, ch),
                               servers[i]->indicatorRenderer(0)));
            w += ch + lquad;
        }

        // Text
        tc.text = servers[i]->longname();
        tc.point = QPointF(w, sy + ascent);
        tc.fg = NormalFg;

        // Background
        if (i == sIdx) {
            tc.fg = ActiveFg;
            m_fills.emplace_back(std::make_pair(QRect(0, sy, 0, lh), ActiveBg));
        }

        w += m_metrics.width(tc.text) + lhalf;
        m_cells.emplace_back(std::move(tc));
        sw = (sw > w) ? sw : w;
        sy += lh;
    }
    if (truncRight) {
        tc.fg = NormalFg;
        tc.text = L("%1 more...").arg(truncRight);
        tc.point = QPointF(0, sy + ascent);
        qreal w = m_metrics.width(tc.text) + lhalf;
        m_cells.emplace_back(std::move(tc));
        sw = (sw > w) ? sw : w;
    }
    if (servers.size() > NITEMS)
        sy += lh;

    for (auto &tc: m_cells) {
        tc.point.rx() -= sw;
    }
    for (auto &elt: m_fills) {
        elt.first.setX(-sw - lquad);
        elt.first.setWidth(sw);
    }
    for (auto &elt: m_icons) {
        elt.first.translate(-sw, 0);
    }

    int nfills = m_fills.size();
    int ncells = m_cells.size();
    int nicons = m_icons.size();

    //
    // Terminal layout
    //
    QVector<TermInstance*> terms;
    const auto &order = m_manager->terms();
    truncLeft = 0, truncRight = 0;
    sIdx = -1;
    qreal tw = 0, ty = 0;
    unsigned nhidden = 0;

    for (int i = tIdx, n = m_manager->totalCount(server); i - tIdx < n; ++i)
        if (!m_manager->isHidden(order[i])) {
            if (order[i] == m_manager->activeTerm())
                sIdx = terms.size();
            terms.append(order[i]);
        } else {
            ++nhidden;
        }

    // Determine the largest number of terminal lines needed
    int nlines = 0;
    bool haveHidden = false;
    for (auto server: servers) {
        if ((tIdx = m_manager->hiddenCount(server)))
            haveHidden = true;
        if ((tIdx = m_manager->totalCount(server) - tIdx) > nlines)
            nlines = tIdx;
    }

    if (nlines > NITEMS) {
        nlines = NITEMS + 2;
        ty = lh;
    } else {
        nlines += haveHidden;
    }

    // Center server list
    qreal th = nlines * lh;
    if (th > sy) {
        sy = (th - sy) / 2;
        for (auto &tc: m_cells)
            tc.point.ry() += sy;
        for (auto &elt: m_fills)
            elt.first.translate(0, sy);
        for (auto &elt: m_icons)
            elt.first.translate(0, sy);
    } else {
        th = sy;
    }

    if (terms.size() > NITEMS) {
        // Term truncation needed
        int onLeft = sIdx;
        int onRight = terms.size() - sIdx - 1;

        if (onLeft < onRight) {
            if (onLeft > NITEMS / 2) {
                truncLeft = onLeft - NITEMS / 2;
            }
            truncRight = terms.size() - truncLeft - NITEMS;
        } else {
            truncLeft = onLeft - NITEMS / 2;
            if (onRight > NITEMS / 2) {
                truncRight = terms.size() - truncLeft - NITEMS;
            } else {
                truncLeft -= NITEMS / 2 - onRight;
            }
        }
    }

    if (truncLeft) {
        tc.fg = NormalFg;
        tc.text = L("%1 more...").arg(truncLeft);
        tc.point = QPointF(lhalf, ascent);
        tw = m_metrics.width(tc.text);
        m_cells.emplace_back(std::move(tc));
    }
    for (int i = truncLeft; i < terms.size() && i - truncLeft < NITEMS; ++i) {
        qreal w = lhalf;

        // Icon
        auto *renderer = terms[i]->iconRenderer();
        if (renderer) {
            m_icons.emplace_back(
                std::make_pair(QRect(lhalf, ty + coct, ch, ch),
                               renderer));
            w += ch + lquad;
        }
        // Indicator(s)
        if (terms[i]->indicatorTypes()[0] != ThumbIcon::InvalidType) {
            m_icons.emplace_back(
                std::make_pair(QRect(w, ty + coct, ch, ch),
                               terms[i]->indicatorRenderer(0)));
            w += ch + lquad;
        }
        if (terms[i]->indicatorTypes()[1] != ThumbIcon::InvalidType) {
            m_icons.emplace_back(
                std::make_pair(QRect(w, ty + coct, ch, ch),
                               terms[i]->indicatorRenderer(1)));
            w += ch + lquad;
        }

        // Text
        tc.text = terms[i]->name();
        tc.point = QPointF(w, ty + ascent);
        tc.fg = NormalFg;

        // Background
        if (i == sIdx) {
            tc.fg = ActiveFg;
            m_fills.emplace_back(
                std::make_pair(QRect(lquad, ty, 0, lh), ActiveBg));
        }

        w += m_metrics.width(tc.text) - lhalf;
        m_cells.emplace_back(std::move(tc));
        tw = (tw > w) ? tw : w;
        ty += lh;
    }
    if (truncRight || nhidden) {
        tc.fg = NormalFg;
        tc.text.clear();
        tc.point = QPointF(lhalf, ty + ascent);
        if (truncRight)
            tc.text = m_text1.arg(truncRight);
        if (nhidden)
            tc.text += m_text2.arg(nhidden);

        qreal w = m_metrics.width(tc.text);
        m_cells.emplace_back(std::move(tc));
        tw = (tw > w) ? tw : w;
    }
    if (nlines > NITEMS || haveHidden)
        ty += lh;

    for (int i = nfills, n = m_fills.size(); i < n; ++i)
        m_fills[i].first.setWidth(tw + lhalf);

    // Center terminals list
    if (th > ty) {
        ty = (th - ty) / 2;
        for (int i = ncells, n = m_cells.size(); i < n; ++i)
            m_cells[i].point.ry() += ty;
        for (int i = nfills, n = m_fills.size(); i < n; ++i)
            m_fills[i].first.translate(0, ty);
        for (int i = nicons, n = m_icons.size(); i < n; ++i)
            m_icons[i].first.translate(0, ty);
    }

    m_bounds.setRect(-sw - 3 * lquad, -lhalf, sw + tw + lh * 2, th + lh);
    m_offset = QPoint(width() / 2, (height() - m_bounds.height()) / 2);
}

inline void
TermPeek::calculateColors()
{
    // Load colors
    const QPalette &pal = palette();
    m_colors[NormalBg] = pal.color(QPalette::Active, QPalette::Window);
    m_colors[NormalFg] = pal.color(QPalette::Active, QPalette::WindowText);
    m_colors[ActiveBg] = pal.color(QPalette::Active, QPalette::Highlight);
    m_colors[ActiveFg] = pal.color(QPalette::Active, QPalette::HighlightedText);
    m_colors[Dark] = pal.color(QPalette::Active, QPalette::Dark);
}

bool
TermPeek::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Polish:
        QWidget::event(event);
        calculateColors();
        calculateCellSize(font());
        return true;
    default:
        return QWidget::event(event);
    }
}

void
TermPeek::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.translate(m_offset);
    painter.fillRect(m_bounds, m_colors[NormalBg]);
    painter.setPen(m_colors[Dark]);
    painter.drawRect(m_bounds);

    for (const auto &i: m_fills)
        painter.fillRect(i.first, m_colors[i.second]);

    for (const auto &cell: m_cells) {
        painter.setPen(m_colors[cell.fg]);
        painter.drawText(cell.point, cell.text);
    }

    for (const auto &i: m_icons)
        i.second->render(&painter, i.first);
}

void
TermPeek::timerEvent(QTimerEvent *)
{
    killTimer(m_timerId);
    m_timerId = 0;
    setVisible(false);
}

void
TermPeek::resizeEvent(QResizeEvent *)
{
    if (m_timerId)
        relayout();
}

void
TermPeek::bringUp()
{
    relayout();

    if (m_timerId)
        killTimer(m_timerId);
    else
        show();

    m_timerId = startTimer(g_global->peekTime());
}
