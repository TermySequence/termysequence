// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/color.h"
#include "app/config.h"
#include "baseinline.h"
#include "imageinline.h"
#include "noteinline.h"
#include "linkinline.h"
#include "seminline.h"
#include "subinline.h"
#include "termwidget.h"
#include "term.h"
#include "buffers.h"
#include "scrollport.h"
#include "manager.h"
#include "selection.h"
#include "settings/global.h"

#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMenu>
#include <QPropertyAnimation>
#include <QtMath>

//
// Base Class
//
InlineBase::InlineBase(Tsqt::RegionType type, TermWidget *parent) :
    QWidget(parent),
    m_type(type),
    m_parent(parent),
    m_term(parent->m_term),
    m_scrollport(parent->m_scrollport)
{
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    setContextMenuPolicy(Qt::PreventContextMenu);

    m_highlightAnim = new QPropertyAnimation(this, B("highlightFade"), this);
    m_highlightAnim->setEndValue(255);
    connect(m_highlightAnim, SIGNAL(finished()), SLOT(handleHighlightFinished()));
    m_actionAnim = new QPropertyAnimation(this, B("actionFade"), this);
    m_actionAnim->setDuration(EFFECT_TIME * 5);
    m_actionAnim->setEndValue(0);

    calculateCellSize();
}

void
InlineBase::calculateCellSize()
{
    QSizeF cellSize = m_parent->cellSize();
    m_cw = cellSize.width();
    m_ch = cellSize.height();

    m_line = 1 + (int)m_ch / CURSOR_BOX_INCREMENT;

    if (m_visible) {
        setRegion(m_region);
        bringUp();
    }

    cellSize.rwidth() *= 2;
    m_icon.setSize(cellSize);
}

void
InlineBase::setRenderFlags(uint8_t flags)
{
    m_renderMask |= flags;
    m_borderMask |= flags;
    update();

    for (auto i: qAsConst(m_subareas)) {
        i->m_renderMask |= flags;
        i->m_borderMask |= flags;
        i->update();
    }
}

void
InlineBase::unsetRenderFlags(uint8_t flags)
{
    m_renderMask &= ~flags;
    m_borderMask &= ~flags;
    update();

    for (auto i: qAsConst(m_subareas)) {
        i->m_renderMask &= ~flags;
        i->m_borderMask &= ~flags;
        i->update();
    }
}

void
InlineBase::setClicking(bool clicking)
{
    m_clicking = clicking;
    update();

    for (auto i: qAsConst(m_subareas)) {
        i->m_clicking = clicking;
        i->update();
    }
}

void
InlineBase::setSelected(bool selected)
{
    if ((m_selected = selected)) {
        m_borderMask |= Selected;
        for (auto i: qAsConst(m_subareas)) {
            i->m_borderMask |= Selected;
            i->update();
        }
    } else {
        m_borderMask &= ~Selected;
        for (auto i: qAsConst(m_subareas)) {
            i->m_borderMask &= ~Selected;
            i->update();
        }
    }
    update();
}

void
InlineBase::setExecuting()
{
    m_actionFade = 6;
    m_actionAnim->start();
    update();

    for (auto i: qAsConst(m_subareas)) {
        i->m_actionFade = 6;
        i->m_actionAnim->start();
        i->update();
    }
}

void
InlineBase::setHighlighting(unsigned duration)
{
    m_highlightFade = (duration + 1) * 255;
    m_highlight = true;
    m_highlightAnim->setDuration(duration * BLINK_TIME);
    m_highlightAnim->start();
    setRenderFlags(Highlighting);

    for (auto i: qAsConst(m_subareas)) {
        i->m_highlightFade = m_highlightFade;
        i->m_highlight = true;
        i->m_highlightAnim->setDuration(duration * BLINK_TIME);
        i->m_highlightAnim->start();
        i->InlineBase::setRenderFlags(Highlighting);
    }
}

void
InlineBase::setHighlightFade(int highlightFade)
{
    m_highlightFade = highlightFade;
    m_highlightAlpha = std::labs(highlightFade % 510 - 255);
    update();
}

void
InlineBase::handleHighlightFinished()
{
    m_highlightAlpha = 255;
    m_highlight = false;
    unsetRenderFlags(Highlighting);
}

void
InlineBase::handleHighlight(regionid_t id)
{
    if (id == INVALID_REGION_ID)
        setHighlighting();
}

void
InlineBase::setActionFade(int actionFade)
{
    m_actionFade = actionFade;
    m_action = actionFade & 1;
    update();
}

void
InlineBase::setBounds(int x, int y, int w, int h)
{
    m_x = x * m_cw;
    m_y = y * m_ch;
    m_w = qCeil(w * m_cw);
    m_h = h * m_ch;

    qreal half = m_line / 2.0;
    m_border.setRect(half, half, m_w - m_line, m_h - m_line);
}

void
InlineBase::setPixelBounds(int y, int w, int h)
{
    m_y = y;
    m_w = w;
    m_h = h;

    qreal half = m_line / 2.0;
    m_border.setRect(half, half, m_w - m_line, m_h - m_line);
}

void
InlineBase::moveIcon()
{
    m_icon.moveRight(m_border.right());
}

void
InlineBase::bringUp()
{
    int y = m_region->startRow + m_region->buffer()->offset() -
        m_region->buffer()->origin() - m_scrollport->offset();

    setGeometry(m_x, m_y + y * m_ch, m_w, m_h);
    raise();
    show();

    for (auto i: qAsConst(m_subareas))
        i->bringUp();
}

void
InlineBase::bringDown()
{
    for (auto i: qAsConst(m_subareas))
        i->bringDown();

    setVisible(false);
    m_selected = false;
    m_renderMask = 0;
    m_borderMask = 0;
}

inline void
InlineBase::checkUrl(const TermUrl &url)
{
    bool selected = !m_url.isEmpty() && m_url == url;
    if (m_selected != selected)
        setSelected(selected);
}

void
InlineBase::setUrl(const TermUrl &url)
{
    m_url = url;
    if (!url.isEmpty() && url == m_scrollport->selectedUrl())
        setSelected(true);
}

void
InlineBase::paintPart1(QPainter &painter)
{
    if (m_clicking || m_action) {
        QColor brush = Colors::blend1(m_term->bg(), m_term->fg());
        brush.setAlpha(120);
        painter.fillRect(rect(), brush);
    } else if (m_highlight) {
        QColor brush = Colors::blend1(m_term->bg(), m_term->fg());
        brush.setAlpha(m_highlightAlpha * 2 / 5);
        painter.fillRect(rect(), brush);
    }
}

void
InlineBase::paintPart2(QPainter &painter)
{
    if (m_renderMask && m_renderer) {
        m_renderer->render(&painter, m_icon);
    }
    if (m_borderMask) {
        QColor color = m_term->bg();
        color.setAlpha(m_highlightAlpha);
        QPen pen(painter.pen());
        pen.setColor(color);
        pen.setWidth(m_line);
        pen.setCapStyle(Qt::FlatCap);
        pen.setJoinStyle(Qt::MiterJoin);
        painter.setPen(pen);
        painter.drawRect(m_border);

        color = m_term->fg();
        color.setAlpha(m_highlightAlpha);
        pen.setStyle(Qt::DashLine);
        pen.setColor(color);
        painter.setPen(pen);
        painter.drawRect(m_border);
    }
}

void
InlineBase::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    paintPart1(painter);
    paintPart2(painter);
}

void
InlineBase::showEvent(QShowEvent *event)
{
    m_visible = true;
    m_mocHighlight = connect(m_scrollport, SIGNAL(semanticHighlight(regionid_t)),
                             SLOT(handleHighlight(regionid_t)));
}

void
InlineBase::hideEvent(QHideEvent *)
{
    if (m_menu) {
        disconnect(m_mocMenu);
        m_menu = nullptr;
    }
    if (m_highlight) {
        m_highlightAnim->stop();
        m_highlightAlpha = 255;
        m_highlight = false;
    }
    if (m_actionFade) {
        m_actionAnim->stop();
        m_actionFade = 0;
        m_action = false;
    }

    disconnect(m_mocHighlight);

    m_visible = false;
    m_clicking = false;
}

void
InlineBase::moveEvent(QMoveEvent *)
{
    if (m_menu) {
        disconnect(m_mocMenu);
        m_menu = nullptr;
    }
    if (m_clicking) {
        setClicking(false);
    }

    unsetRenderFlags(Menuing|Hovering);
}

void
InlineBase::resizeEvent(QResizeEvent *)
{
    moveEvent(nullptr);
}

void
InlineBase::mouseAction(QMouseEvent *event, int action)
{
    switch (action) {
    case InlineActOpen:
        open();
        break;
    case InlineActSelect:
        textSelect();
        break;
    case InlineActCopy:
        clipboardCopy();
        break;
    case InlineActWrite:
        textWrite();
        break;
    default:
        event->ignore();
    }
}

void
InlineBase::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton &&
        (!m_parent->m_mouseReport || event->modifiers() & NoReportMods))
    {
        mouseAction(event, g_global->inlineAction0());
    }
    else {
        event->ignore();
    }
}

void
InlineBase::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton &&
        (!m_parent->m_mouseReport || event->modifiers() & NoReportMods))
    {
        m_dragStartPosition = event->pos();
        setClicking(true);
        if (!m_url.isEmpty())
            m_scrollport->setSelectedUrl(m_url);
        m_parent->m_blink->setBlinkEffect(m_scrollport);
    }
    else {
        event->ignore();
    }
}

void
InlineBase::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton &&
        (!m_parent->m_mouseReport || event->modifiers() & NoReportMods))
    {
        if (m_clicking) {
            setClicking(false);
            if (event->modifiers() == Qt::ControlModifier)
                mouseAction(event, g_global->inlineAction1());
            else if (event->modifiers() == Qt::ShiftModifier)
                mouseAction(event, g_global->inlineAction2());
        }
    }
    else {
        event->ignore();
    }
}

void
InlineBase::mouseMoveEvent(QMouseEvent *event)
{
    if (m_clicking) {
        int dist = (event->pos() - m_dragStartPosition).manhattanLength();
        if (dist >= g_global->inlineDrag() * QApplication::startDragDistance())
        {
            setClicking(false);
            QMimeData *data = new QMimeData;
            QDrag *drag = new QDrag(m_parent);

            getDragData(data, drag);

            drag->setMimeData(data);
            drag->exec(Qt::CopyAction);
        }
    }
    else {
        event->ignore();
    }
}

void
InlineBase::enterEvent(QEvent *)
{
    // Avoid hover when selecting
    if (!m_scrollport->selecting())
        setRenderFlags(Hovering);
}

void
InlineBase::leaveEvent(QEvent *)
{
    unsetRenderFlags(Hovering);
}

void
InlineBase::handleMenuFinished()
{
    unsetRenderFlags(Menuing);
    m_menu = nullptr;
}

QMenu *
InlineBase::getInlinePopup(MainWindow *window)
{
    if (!m_url.isEmpty())
        m_scrollport->setSelectedUrl(m_url);
    m_menu = getPopup(window);
    setRenderFlags(Menuing);
    m_mocMenu = connect(m_menu, SIGNAL(aboutToHide()), SLOT(handleMenuFinished()));
    return m_menu;
}

void
InlineBase::clipboardCopy()
{
    int n = m_region->clipboardCopy();
    m_scrollport->manager()->reportClipboardCopy(n);
}

void
InlineBase::textSelect()
{
    m_term->buffers()->selection()->selectRegion(m_region);
}

void
InlineBase::textWrite()
{
    QString text = m_region->getLine();
    if (!text.isEmpty()) {
        m_term->pushInput(m_scrollport->manager(), text);
    }
}

//
// Widget Cache
//
InlineCache::InlineCache(TermWidget *parent) :
    m_parent(parent)
{
}

void
InlineCache::setupWidget(const Region *region, InlineBase *base)
{
    base->setRegion(region);

    InlineSubarea *sub;
    int i, n = base->m_subrects.size();

    while (base->m_subareas.size() > n) {
        sub = base->m_subareas.takeLast();
        sub->bringDown();
        m_inactiveSubs.insert(sub);
    }
    for (i = 0; i < base->m_subareas.size(); ++i) {
        sub = base->m_subareas[i];
        sub->setSubRegion(base, i);
    }
    for (; i < n; ++i) {
        if (m_inactiveSubs.empty())
            sub = new InlineSubarea(m_parent);
        else {
            auto i = m_inactiveSubs.begin();
            sub = *i;
            m_inactiveSubs.erase(i);
        }
        base->m_subareas.append(sub);
        sub->setSubRegion(base, i);
    }

    base->bringUp();
}

void
InlineCache::getWidget(const Region *region, InlineMap &addto)
{
    RegionKey key = std::make_pair(region->type(), region->id());
    auto i = m_active.find(key);
    if (i != m_active.end()) {
        i->second->bringUp();
        addto.insert(*i);
        m_active.erase(i);
        return;
    }

    for (auto i = m_inactive.begin(), j = m_inactive.end(); i != j; ++i)
        if ((*i)->type() == region->type()) {
            setupWidget(region, *i);
            addto.emplace(key, *i);
            m_inactive.erase(i);
            return;
        }

    InlineBase *base;

    switch (region->type()) {
    case Tsqt::RegionImage:
        base = new InlineImage(m_parent);
        break;
    case Tsqt::RegionUser:
        base = new InlineNote(m_parent);
        break;
    case Tsqt::RegionLink:
        base = new InlineLink(m_parent);
        break;
    default:
        base = new InlineSemantic(m_parent);
    }

    setupWidget(region, base);
    addto.emplace(key, base);
}

void
InlineCache::process(const RegionList &regions)
{
    InlineMap nextactive;

    for (const RegionRef &region: regions.list)
        if (region.cref()->flags & Tsqt::Inline)
            getWidget(region.cref(), nextactive);

    for (const auto &i: m_active) {
        auto *base = i.second;
        base->bringDown();
        m_inactive.insert(base);
        m_inactiveSubs.insert(base->m_subareas.begin(), base->m_subareas.end());
        base->m_subareas.clear();
    }

    m_active.swap(nextactive);
}

void
InlineCache::updateUrl(const TermUrl &url)
{
    for (const auto &i: m_active)
        i.second->checkUrl(url);
}

void
InlineCache::calculateCellSize()
{
    for (const auto &i: m_active)
        i.second->calculateCellSize();
    for (auto i: m_inactive)
        i->calculateCellSize();
}
