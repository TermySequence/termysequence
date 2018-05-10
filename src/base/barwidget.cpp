// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "app/logging.h"
#include "barwidget.h"
#include "serverarrange.h"
#include "thumbarrange.h"
#include "server.h"
#include "term.h"
#include "manager.h"
#include "settings/global.h"

#include <QDropEvent>
#include <QScrollBar>
#include <QApplication>
#include <QDesktopWidget>
#include <QMimeData>

//
// Contents Widget
//
BarWidget::BarWidget(TermManager *manager, BarScroll *parent, const QSize &limit) :
    QWidget(parent),
    m_manager(manager),
    m_parent(parent),
    m_sizeLimit(limit)
{
    setAcceptDrops(true);

    connect(manager, SIGNAL(serverAdded(ServerInstance*)), SLOT(handleServerAdded(ServerInstance*)));
    connect(manager, SIGNAL(serverRemoved(ServerInstance*)), SLOT(handleServerRemoved(ServerInstance*)));
    connect(manager, SIGNAL(serverHidden(ServerInstance*,size_t,size_t)), SLOT(relayout()));
    connect(manager, SIGNAL(termAdded(TermInstance*)), SLOT(handleTermAdded(TermInstance*)));
    connect(manager, SIGNAL(termRemoved(TermInstance*,TermInstance*)), SLOT(handleTermRemoved(TermInstance*)));
    connect(manager, SIGNAL(termActivated(TermInstance*,TermScrollport*)), SLOT(handleTermActivated(TermInstance*)));
    connect(manager, SIGNAL(termReordered()), SLOT(relayout()));
    connect(g_global, SIGNAL(denseThumbChanged()), SLOT(handleSettingsChanged()));
    connect(g_global, SIGNAL(termCaptionChanged(QString)), SLOT(handleSettingsChanged()));

    m_dense = g_global->denseThumb() || g_global->termCaption().isEmpty();

    for (auto server: manager->servers()) {
        ServerArrange *widget = new ServerArrange(server, manager, this);
        m_widgetMap.insert(server, widget);
    }
    for (auto term: manager->terms()) {
        ThumbArrange *widget = new ThumbArrange(term, manager, this);
        m_widgetMap.insert(term, widget);
    }
}

void
BarWidget::handleSettingsChanged()
{
    m_dense = g_global->denseThumb() || g_global->termCaption().isEmpty();
    relayout();
}

void
BarWidget::handleServerAdded(ServerInstance *server)
{
    ServerArrange *widget = new ServerArrange(server, m_manager, this);
    m_widgetMap.insert(server, widget);
    relayout();
    widget->show();
}

void
BarWidget::handleServerRemoved(ServerInstance *server)
{
    auto i = m_widgetMap.constFind(server);
    if (i != m_widgetMap.cend()) {
        stopDrag();
        delete *i;
        m_widgetMap.erase(i);
        relayout();
    }
}

void
BarWidget::handleTermAdded(TermInstance *term)
{
    ThumbArrange *widget = new ThumbArrange(term, m_manager, this);
    m_widgetMap.insert(term, widget);
    relayout();
    widget->show();
}

void
BarWidget::handleTermRemoved(TermInstance *term)
{
    auto i = m_widgetMap.constFind(term);
    if (i != m_widgetMap.cend()) {
        stopDrag();
        delete *i;
        m_widgetMap.erase(i);
        relayout();
    }
}

void
BarWidget::handleTermActivated(TermInstance *term)
{
    auto i = m_widgetMap.constFind(term), j = m_widgetMap.cend();
    if (i == j)
        return;

    ThumbBase *thumb = *i;
    m_parent->ensureWidgetVisible(thumb);

    i = m_widgetMap.constFind(term->server());
    if (i == j)
        return;

    if (m_horizontal ?
        thumb->x() + thumb->width() - (*i)->x() < m_parent->viewport()->width() :
        thumb->y() + thumb->height() - (*i)->y() < m_parent->viewport()->height())
        m_parent->ensureWidgetVisible(*i, 0, 0);
}

void
BarWidget::placeHorizontal(ThumbBase *widget, QRect &bounds, int height)
{
    if (widget->hidden()) {
        widget->hide();
    } else {
        int l = m_sizeLimit.width();
        int w = widget->widthForHeight(height);

        if (w > l)
            w = l;
        else if (!m_dense && w < (l/2))
            w = l/2;

        bounds.setWidth(w);
        widget->setGeometry(bounds);
        widget->show();
        bounds.translate(w, 0);
    }
}

void
BarWidget::placeVertical(ThumbBase *widget, QRect &bounds, int width)
{
    if (widget->hidden()) {
        widget->hide();
    } else {
        int l = m_sizeLimit.height();
        int h = widget->heightForWidth(width);

        if (h > l)
            h = l;
        else if (h < (l/2))
            h = l/2;

        bounds.setHeight(h);
        widget->setGeometry(bounds);
        widget->show();
        bounds.translate(0, h);
    }
}

void
BarWidget::relayoutHorizontal()
{
    int h = height(), l = m_sizeLimit.height(), offset = 0;
    if (h > l) {
        offset = (h - l) / 2;
        h = l;
    }

    QRect bounds(0, offset, 0, h);

    for (auto i: m_manager->order())
        placeHorizontal(m_widgetMap[i], bounds, h);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    m_sizeHint = QSize(bounds.x(), h);
    resize(bounds.x(), height());
    m_horizontal = true;
}

void
BarWidget::relayoutVertical()
{
    int w = width(), l = m_sizeLimit.width(), offset = 0;
    if (w > l) {
        offset = (w - l) / 2;
        w = l;
    }

    QRect bounds(offset, 0, w, 0);

    for (auto i: m_manager->order())
        placeVertical(m_widgetMap[i], bounds, w);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    m_sizeHint = QSize(w, bounds.y());
    resize(width(), bounds.y());
    m_horizontal = false;
}

void
BarWidget::relayout()
{
    qCDebug(lcLayout) << this << "relayout at" << size();

    // Determine orientation
    if (width() > height()) {
        relayoutHorizontal();
    } else {
        relayoutVertical();
    }

    updateGeometry();
}

inline ThumbBase *
BarWidget::findDragTarget(QPoint dragPos)
{
    QWidget *child = childAt(dragPos);

    while (child && child->parentWidget() != this)
        child = child->parentWidget();

    return qobject_cast<ThumbBase*>(child);
}

inline void
BarWidget::stopDrag()
{
    if (m_timerId) {
        killTimer(m_timerId);
        m_timerId = 0;
    }
    m_dragDirection = 0;
    m_dragSource.clear();
    m_dragTargetId.clear();

    if (m_dragTargets[0]) {
        m_dragTargets[0]->clearDrag();
        m_dragTargets[0] = nullptr;
    }
    if (m_dragTargets[1]) {
        m_dragTargets[1]->clearDrag();
        m_dragTargets[1] = nullptr;
    }
}

void
BarWidget::updateDragAutoScroll(QPoint dragPos)
{
    // update autoscroll
    QPoint p = mapToParent(dragPos);
    int dragDirection = 0;

    if (m_horizontal) {
        if (p.x() < DRAG_AUTOSCROLL_MARGIN)
            dragDirection = -1;
        else if (p.x() > parentWidget()->width() - DRAG_AUTOSCROLL_MARGIN)
            dragDirection = 1;
    } else {
        if (p.y() < DRAG_AUTOSCROLL_MARGIN)
            dragDirection = -1;
        else if (p.y() > parentWidget()->height() - DRAG_AUTOSCROLL_MARGIN)
            dragDirection = 1;
    }

    if (dragDirection == 0) {
        m_dragDirection = 0;
        if (m_timerId) {
            killTimer(m_timerId);
            m_timerId = 0;
        }
    } else if (dragDirection != m_dragDirection) {
        m_dragDirection = dragDirection;
        if (!m_timerId) {
            m_timerId = startTimer(DRAG_SCROLL_TIME);
        }
    }
}

void
BarWidget::updateReorderDragHelper(QPoint dragPos, ThumbBase *dragTarget)
{
    QPoint p = dragTarget->mapFrom(this, dragPos);
    bool reorderAfter = m_horizontal ?
        p.x() >= dragTarget->width() / 2 :
        p.y() >= dragTarget->height() / 2;

    if (m_reorderAfter != reorderAfter || m_dragTargetId != dragTarget->idStr())
    {
        QObject *obj[2];

        m_dragTargetId = dragTarget->idStr();
        m_reorderAfter = reorderAfter;
        m_manager->findDragTargets(m_dragSource, m_dragTargetId, reorderAfter, obj);

        if (m_dragTargets[0])
            m_dragTargets[0]->clearDrag();
        if (m_dragTargets[1])
            m_dragTargets[1]->clearDrag();

        m_dragTargets[0] = m_widgetMap.value(obj[0]);
        m_dragTargets[1] = m_widgetMap.value(obj[1]);

        if (m_dragTargets[0])
            m_dragTargets[0]->setReorderDrag(false);
        if (m_dragTargets[1])
            m_dragTargets[1]->setReorderDrag(true);
    }
}

inline ThumbBase *
BarWidget::updateReorderDrag(QDropEvent *event)
{
    ThumbBase *dragTarget = findDragTarget(event->pos());
    m_dragSource = event->mimeData()->data(REORDER_MIME_TYPE);
    m_reorderDrag = true;

    if (dragTarget) {
        updateReorderDragHelper(event->pos(), dragTarget);
        updateDragAutoScroll(event->pos());
    }

    event->acceptProposedAction();
    return dragTarget;
}

ThumbBase *
BarWidget::updateNormalDrag(QDropEvent *event)
{
    ThumbBase *dragTarget = findDragTarget(event->pos());
    m_reorderDrag = false;

    if (!dragTarget)
    {
        m_dragTargetId.clear();

        if (m_dragTargets[0]) {
            m_dragTargets[0]->clearDrag();
            m_dragTargets[0] = nullptr;
        }

        m_normalDragOk = false;
    }
    else if (m_dragTargetId != dragTarget->idStr())
    {
        m_dragTargetId = dragTarget->idStr();

        if (m_dragTargets[0])
            m_dragTargets[0]->clearDrag();

        m_dragTargets[0] = dragTarget;
        m_normalDragOk = dragTarget->setNormalDrag(event->mimeData());
    }

    if (dragTarget)
        updateDragAutoScroll(event->pos());

    if (m_normalDragOk) {
        event->acceptProposedAction();
    } else {
        event->setDropAction(Qt::IgnoreAction);
        event->accept();
    }

    return dragTarget;
}

void
BarWidget::timerEvent(QTimerEvent *)
{
    if (m_dragDirection != 0)
    {
        m_parent->scroll(m_dragDirection, m_horizontal);

        if (m_reorderDrag) {
            QPoint dragPos = mapFromGlobal(QCursor::pos());
            ThumbBase *dragTarget = findDragTarget(dragPos);

            if (dragTarget)
                updateReorderDragHelper(dragPos, dragTarget);
        }
    }
}

void
BarWidget::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *data = event->mimeData();

    if (data->hasFormat(REORDER_MIME_TYPE)) {
        Tsq::Uuid tmp;
        if (tmp.parse(data->data(REORDER_MIME_TYPE).data()))
            updateReorderDrag(event);
    }
    else {
        updateNormalDrag(event);
    }
}

void
BarWidget::dragLeaveEvent(QDragLeaveEvent *)
{
    stopDrag();
}

void
BarWidget::dragMoveEvent(QDragMoveEvent *event)
{
    const QMimeData *data = event->mimeData();

    if (data->hasFormat(REORDER_MIME_TYPE)) {
        updateReorderDrag(event);
    }
    else {
        updateNormalDrag(event);
    }
}

void
BarWidget::dropEvent(QDropEvent *event)
{
    const QMimeData *data = event->mimeData();

    if (data->hasFormat(REORDER_MIME_TYPE)) {
        updateReorderDrag(event);
        m_manager->moveObject(data->data(REORDER_MIME_TYPE),
                              m_dragTargetId, m_reorderAfter);
    }
    else {
        ThumbBase *dragTarget = updateNormalDrag(event);
        if (dragTarget)
            dragTarget->dropNormalDrag(data);
    }

    stopDrag();
}

bool
BarWidget::event(QEvent *event)
{
    if (event->type() == QEvent::LayoutRequest) {
        relayout();
        return true;
    }

    return QWidget::event(event);
}

QSize
BarWidget::sizeHint() const
{
    return m_sizeHint;
}

//
// Scroll Area
//
BarScroll::BarScroll(TermManager *manager)
{
    setFrameStyle(0);
    setFocusPolicy(Qt::NoFocus);

    // Calculate size limits
    QSize size = QApplication::desktop()->availableGeometry(this).size() / THUMB_MIN_SCALE;

    if (size.width() < THUMB_MIN_SIZE)
        size.setWidth(THUMB_MIN_SIZE);
    if (size.height() < THUMB_MIN_SIZE)
        size.setHeight(THUMB_MIN_SIZE);

    setMinimumSize(size);
    m_sizeHint = size * 1.2;

    // Create child widget
    m_widget = new BarWidget(manager, this, size * 2);
    setWidget(m_widget);
    setWidgetResizable(true);
}

void
BarScroll::resizeEvent(QResizeEvent *event)
{
    if (width() > height()) {
        // Horizontal layout
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        QScrollArea::resizeEvent(event);
        m_widget->relayoutHorizontal();
    } else {
        // Vertical layout
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        QScrollArea::resizeEvent(event);
        m_widget->relayoutVertical();
    }

    // m_widget->relayout();
}

QSize
BarScroll::sizeHint() const
{
    return m_sizeHint;
}

void
BarScroll::scroll(int direction, bool horizontal)
{
    QScrollBar *bar = horizontal ? horizontalScrollBar() : verticalScrollBar();
    bar->setValue(bar->value() + direction * bar->singleStep());
}
