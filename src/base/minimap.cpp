// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/color.h"
#include "app/config.h"
#include "minimap.h"
#include "scrollport.h"
#include "term.h"
#include "buffers.h"
#include "stack.h"
#include "screen.h"
#include "selection.h"
#include "mark.h"
#include "pin.h"
#include "settings/profile.h"

#include <QPainter>
#include <QMouseEvent>
#include <QtMath>

TermMinimap::TermMinimap(TermScrollport *scrollport, QWidget *parent) :
    QWidget(parent),
    m_scrollport(scrollport),
    m_term(scrollport->term()),
    m_buffers(m_term->buffers())
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    connect(m_term, SIGNAL(stacksChanged()), SLOT(updateStacks()));
    connect(m_term, SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));
    connect(m_term, SIGNAL(colorsChanged(QRgb,QRgb)), SLOT(recolor(QRgb,QRgb)));
    connect(m_term, SIGNAL(paletteChanged()), SLOT(handleRegions()));
    connect(m_term, SIGNAL(miscSettingsChanged()), SLOT(handleSettingsChanged()));
    connect(m_buffers, SIGNAL(regionChanged()), SLOT(handleRegions()));
    connect(m_buffers, SIGNAL(bufferReset()), SLOT(handleRegions()));
    connect(m_buffers, SIGNAL(bufferChanged()), SLOT(updateScroll()));
    connect(m_buffers, SIGNAL(fetchPosChanged()), SLOT(updateFetchPos()));
    connect(m_scrollport, SIGNAL(offsetChanged(int)), SLOT(updateScroll()));

    m_recentPrompts = m_term->profile()->recentPrompts();
    m_showPos = m_term->profile()->fetchPos();

    refont(m_term->font());
    recolor(m_term->bg(), m_term->fg());
}

void
TermMinimap::placePin(TermMinipin *pin)
{
    size_t size = m_buffers->size();
    index_t origin = m_buffers->origin();

    if (size == 0 || m_term->overlayActive()) {
        pin->setGeometry(0, 0, 0, 0);
        return;
    }

    qreal h = height();
    qreal start = h * (pin->startRow() - origin) / size;
    qreal end = h * (pin->endRow() - origin) / size;
    h = end - start;
    qreal d = m_cellSize.height() / 2.0;

    if (pin->regionType() == Tsqt::RegionUser) {
        start += (h - m_cellSize.height()) / 2.0;
        h = m_cellSize.height();
    }
    else if (h < d) {
        start += (h - d) / 2.0;
        h = d;
    }

    pin->setGeometry(2, start, width() - 4, h);
}

void
TermMinimap::handleRegion(const Region *r, int &index)
{
    TermMinipin *pin;

    if (index == m_pins.size()) {
        m_pins.append(pin = new TermMinipin(m_scrollport, this));
    } else {
        pin = m_pins.at(index);
    }

    if (pin->setRegion(r)) {
        placePin(pin);
        pin->raise();
        pin->show();
        ++index;
    } else {
        pin->setVisible(false);
    }
}

void
TermMinimap::handleRegions()
{
    if (!m_visible)
        return;

    const Selection *selection = m_buffers->selection();
    const Region **buf = new const Region *[m_recentPrompts];
    int index = 0;

    // First, the active selection
    if (!selection->isEmpty()) {
        handleRegion(selection, index);
    }

    // Next, the recent prompts
    int n = m_buffers->recentJobs(buf, m_recentPrompts);
    while (n > 0) {
        handleRegion(buf[--n], index);
    }

    // Finally, all active regions
    for (auto i: m_buffers->activeRegions()) {
        handleRegion(i, index);
    }

    if (m_hiddenThreshold != index) {
        int tmp = m_hiddenThreshold;
        m_hiddenThreshold = index;

        for (; index < tmp; ++index) {
            m_pins.at(index)->setVisible(false);
        }
    }

    delete [] buf;
}

void
TermMinimap::calculateBounds(TermScrollport *scrollport, QRectF &bounds)
{
    size_t offset = scrollport->offset();
    size_t pageSize = scrollport->height();
    size_t totalSize = m_buffers->size();
    qreal h = height();

    if (totalSize == 0 || m_term->overlayActive()) {
        offset = 0;
        totalSize = pageSize;
    }

    bounds.setRect(0, h * offset / totalSize - m_radius,
                   width(), h * pageSize / totalSize + m_radius * 2.0);

    if (m_scrollport != scrollport)
        bounds.adjust(m_line, m_line, -m_line, -m_line);
}

void
TermMinimap::updateStacks()
{
    for (auto i = m_others.cbegin(), j = m_others.cend(); i != j; ++i)
        disconnect(i.value().moc);

    m_others.clear();

    for (const auto stack: m_term->stacks())
    {
        const auto scrollport = stack->currentScrollport();
        if (m_scrollport != scrollport) {
            OtherRec &rec = m_others[scrollport];
            calculateBounds(scrollport, rec.bounds);
            rec.index = QString::number(stack->index());
            rec.moc = connect(scrollport, SIGNAL(offsetChanged(int)), SLOT(updateOther()));
        } else {
            m_index = QString::number(stack->index());
        }
    }

    update();
}

void
TermMinimap::updateOther()
{
    if (m_visible) {
        auto *scrollport = static_cast<TermScrollport*>(sender());
        calculateBounds(scrollport, m_others[scrollport].bounds);
        update();
    }
}

void
TermMinimap::updateScroll()
{
    if (m_visible) {
        calculateBounds(m_scrollport, m_bounds);
        update();

        for (int i = 0; i < m_hiddenThreshold; ++i) {
            placePin(m_pins.at(i));
        }
    }
}

void
TermMinimap::updateFetchPos()
{
    unsigned screenHeight = m_term->screen()->height();
    index_t start = m_buffers->origin();
    index_t end = start + m_buffers->size0() - screenHeight;
    size_t fetchPos = m_buffers->fetchPos();
    size_t size = m_buffers->size();

    // Don't show marker if it is within one screenHeight of the top
    end -= (end > screenHeight) ? screenHeight : end;

    if (fetchPos < start || fetchPos >= end || size == 0) {
        m_havePos = false;
    } else {
        m_havePos = true;
        qreal pos = height() * (fetchPos - start) / size;
        qreal w = width();

        m_posRect.setRect(0, pos, width(), m_cellSize.height() / 8);
        m_posTri[0] = QPointF(w / 2, pos);
        m_posTri[1] = QPointF(w / 4, pos - m_cellSize.height() / 2);
        m_posTri[2] = QPointF(w * 3 / 4, pos - m_cellSize.height() / 2);
    }

    if (m_showPos)
        update();
}

void
TermMinimap::handleSettingsChanged()
{
    m_recentPrompts = m_term->profile()->recentPrompts();
    m_showPos = m_term->profile()->fetchPos();
    handleRegions();
    update();
}

void
TermMinimap::recolor(QRgb bg, QRgb fg)
{
    m_fg = fg;
    m_bg = Colors::blend1(bg, fg);
    m_blend = Colors::blend2(bg, fg);

    handleRegions();
    update();
}

void
TermMinimap::refont(const QFont &font)
{
    calculateCellSize(m_font = font);
    m_line = 1 + m_cellSize.height() / MARGIN_INCREMENT;

    int w = qCeil(2 * m_cellSize.width());

    m_sizeHint = QSize(w, w);
    updateGeometry();
}

void
TermMinimap::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), m_bg);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setFont(m_font);

    if (!m_others.isEmpty()) {
        QPen pen(m_blend);
        pen.setWidth(m_line * 2);
        painter.setBrush(Qt::NoBrush);

        for (auto i = m_others.cbegin(), j = m_others.cend(); i != j; ++i) {
            painter.setPen(pen);
            painter.drawRoundedRect(i.value().bounds, m_radius, m_radius);
            painter.setPen(m_fg);
            painter.drawText(i.value().bounds, Qt::AlignCenter, i.value().index);
        }
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(m_blend);
    painter.drawRoundedRect(m_bounds, m_radius, m_radius);
    painter.setPen(m_fg);
    painter.drawText(m_bounds, Qt::AlignCenter, m_index);

    if (m_havePos && m_showPos && !m_term->overlayActive()) {
        painter.setBrush(m_fg);
        painter.fillRect(m_posRect, m_fg);
        painter.drawPolygon(m_posTri, 3);
    }
}

void
TermMinimap::resizeEvent(QResizeEvent *event)
{
    m_radius = width() / 2.0;

    updateScroll();
    updateStacks();
    updateFetchPos();
}

void
TermMinimap::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        int y = event->y();
        int h = height();

        if (y >= h)
            y = h - 1;
        if (y < 0)
            y = 0;
        if (h == 0)
            h = 1;

        unsigned half = m_scrollport->height() / 2;

        size_t offset = m_buffers->size() * y / h;
        offset = (offset > half) ? offset - half : 0l;

        m_scrollport->scrollToRow(m_buffers->origin() + offset, true);
        event->accept();
    }
}

void
TermMinimap::mouseMoveEvent(QMouseEvent *event)
{
    mousePressEvent(event);
}

void
TermMinimap::showEvent(QShowEvent *event)
{
    m_visible = true;
    updateStacks();
    updateScroll();
    handleRegions();
}

void
TermMinimap::hideEvent(QHideEvent *event)
{
    m_visible = false;
}

QSize
TermMinimap::sizeHint() const
{
    return m_sizeHint;
}

QSize
TermMinimap::minimumSizeHint() const
{
    return QSize();
}
