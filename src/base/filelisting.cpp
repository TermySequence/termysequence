// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "filelisting.h"
#include "filemodel.h"
#include "fileviewitem.h"
#include "filewidget.h"
#include "file.h"
#include "settings/global.h"

#include <QScrollBar>
#include <QMouseEvent>
#include <QPainter>
#include <QMenu>
#include <QApplication>
#include <QtMath>

//
// Scroll area
//
FileScroll::FileScroll(FileNameItem *item) :
    m_nameitem(item)
{
    setFocusPolicy(Qt::NoFocus);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

void
FileScroll::setWidget(QWidget *widget)
{
    QScrollArea::setWidget(widget);
    setWidgetResizable(true);
}

void
FileScroll::scrollContentsBy(int dx, int dy)
{
    m_nameitem->setBlinkEffect();
    QScrollArea::scrollContentsBy(dx, dy);
}

//
// List view
//
FileListing::FileListing(FileFilter *filter, FileNameItem *item,
                         FileWidget *parent, FileScroll *scroll) :
    m_filter(filter),
    m_nameitem(item),
    m_parent(parent),
    m_scroll(scroll)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    setMouseTracking(true);

    connect(filter->model(), SIGNAL(fontChanged(const QFont&)), SLOT(refont(const QFont&)));

    connect(filter,
            SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&,const QVector<int>&)),
            SLOT(handleDataChanged(const QModelIndex&,const QModelIndex&,const QVector<int>&)));
    connect(filter, SIGNAL(modelReset()), SLOT(handleModelReset()));
    connect(filter,
            SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            SLOT(handleRowsInserted(const QModelIndex&,int,int)));
    connect(filter,
            SIGNAL(rowsRemoved(const QModelIndex&,int,int)),
            SLOT(handleRowsRemoved(const QModelIndex&,int,int)));
    connect(filter,
            SIGNAL(layoutAboutToBeChanged(const QList<QPersistentModelIndex>&)),
            SLOT(handleLayoutChanging(const QList<QPersistentModelIndex>&)));
    connect(filter,
            SIGNAL(layoutChanged(const QList<QPersistentModelIndex>&)),
            SLOT(handleLayoutChanged(const QList<QPersistentModelIndex>&)));

    installEventFilter(parent);
    m_scroll->verticalScrollBar()->installEventFilter(parent);
}

FileListing::~FileListing()
{
    delete [] m_widths;
}

inline bool
FileListing::calculateColumnWidths(int total, int cols)
{
    if (total < cols)
        return false;

    int *widths = new int[cols];
    int *count = new int[cols];

    int rem = total % cols;
    int row = 0;
    for (int i = 0; i < cols; ++i)
    {
        if (rem) {
            ++row;
            --rem;
        }
        count[i] = (row += total / cols);
    }

    row = 0;
    int result = 0;
    int limit = width();
    for (int i = 0; i < cols; ++i)
    {
        int max = 0;
        for (; row < count[i]; ++row) {
            int width = m_list.at(row).first.width();

            if (max < width) {
                max = width;
                if (result + max > limit) {
                    delete [] count;
                    goto out;
                }
            }
        }

        widths[i] = max;
        result += max;
    }

    delete [] count;
    result += 2 * (cols - 1) * m_cellSize.width();
    if (result <= limit) {
        delete [] m_widths;
        m_widths = widths;
        return true;
    }
out:
    delete [] widths;
    return false;
}

inline void
FileListing::positionRectangles(int total)
{
    int *count = new int[m_cols];

    int rem = total % m_cols;
    int row = 0;
    for (int i = 0; i < m_cols; ++i)
    {
        if (rem) {
            ++row;
            --rem;
        }
        count[i] = (row += total / m_cols);
    }

    row = 0;
    int x = 0;
    for (int i = 0; i < m_cols; ++i)
    {
        int y = 0;
        for (; row < count[i]; ++row) {
            auto &ref = m_list[row];
            ref.first.moveTo(x, y);
            (ref.second = ref.first).setWidth(m_widths[i]);
            y += m_cellSize.height();
        }

        x += m_widths[i] + 2 * m_cellSize.width();
    }

    delete [] count;
}

inline int
FileListing::computeWidth(const TermFile *file)
{
    qreal w = m_nameitem->stringPixelWidth(file->name);

    if (file->classify && file->fileclass)
        w += m_cellSize.width();

    if (file->gittify) {
        if (file->gitflags)
            w += m_cellSize.width() * 1.5;
        if ((file->gitflags & Tsqt::GitStatusAnyIndex) &&
            (file->gitflags & Tsqt::GitStatusAnyWorking))
            w += m_cellSize.width();
    }

    return qCeil(w);
}

void
FileListing::relayout(bool restart)
{
    if (!m_visible) {
        m_restart |= restart;
        return;
    }

    int n = m_filter->rowCount();

    if (restart) {
        for (int i = 0; i < n; ++i)
        {
            const auto *file = FILE_FILEP(m_filter->index(i, 0));
            int w = computeWidth(file);

            m_list[i].first.setSize(QSize(w, m_cellSize.height()));
        }
    }

    // Compute the number of columns
    for (m_cols = 1; calculateColumnWidths(n, m_cols + 1); ++m_cols);
    if (m_cols == 1) {
        delete [] m_widths;
        m_widths = new int[1];
        *m_widths = width();
    }

    // Translate rectangles
    positionRectangles(n);

    n = m_cellSize.height() * (n + m_cols - 1) / m_cols;
    m_sizeHint.setHeight(n > 0 ? n : 1);
    setMinimumSize(m_sizeHint);
    updateGeometry();
    update();
}

void
FileListing::handleDataChanged(const QModelIndex &si, const QModelIndex &ei,
                               const QVector<int> &roles)
{
    if (si.column() == FILE_COLUMN_MODE || si.column() == FILE_COLUMN_NAME)
    {
        if (roles.isEmpty()) {
            int start = si.row();
            int end = ei.row();

            while (end >= start) {
                const auto *file = FILE_FILEP(m_filter->index(end, 0));
                int w = computeWidth(file);

                m_list[end].first.setWidth(w);
                --end;
            }

            relayout(false);
        }
        else {
            update();
        }
    }
}

void
FileListing::handleRowsInserted(const QModelIndex &, int start, int end)
{
    if (m_selected >= start) {
        m_selected += (end - start) + 1;
        m_parent->updateSelection(m_selected);
    }

    while (end >= start) {
        const auto *file = FILE_FILEP(m_filter->index(end, 0));
        int w = computeWidth(file);

        m_list.insert(start, std::make_pair(QRect(),QRect()));
        m_list[start].first.setSize(QSize(w, m_cellSize.height()));
        --end;
    }

    relayout(false);
}

void
FileListing::handleRowsRemoved(const QModelIndex &, int start, int end)
{
    if (m_selected > end) {
        m_selected -= (end - start) + 1;
        m_parent->updateSelection(m_selected);
    }
    else if (m_selected >= start) {
        m_selected = -1;
        m_parent->updateSelection(-1);
    }

    while (end-- >= start)
        m_list.removeAt(start);

    relayout(false);
}

void
FileListing::handleLayoutChanging(const QList<QPersistentModelIndex> &parents)
{
    if (parents.isEmpty() && m_selected != -1) {
        auto index = m_filter->index(m_selected, FILE_COLUMN_NAME);
        m_savedSelection = index.data().toString();
    }
}

void
FileListing::handleLayoutChanged(const QList<QPersistentModelIndex> &)
{
    if (!m_savedSelection.isEmpty()) {
        m_selected = -1;
        for (int i = 0, n = m_filter->rowCount(); i < n; ++i) {
            auto index = m_filter->index(i, FILE_COLUMN_NAME);
            if (m_savedSelection == index.data().toString()) {
                m_selected = i;
                break;
            }
        }
        m_parent->updateSelection(m_selected);
        m_savedSelection.clear();
    }

    if (!m_filter->model()->animating()) {
        m_list.resize(m_filter->rowCount());
        relayout(true);
    }
}

void
FileListing::handleModelReset()
{
    m_selected = -1;
    m_list.resize(m_filter->rowCount());
    relayout(true);
}

void
FileListing::refont(const QFont &font)
{
    if (m_font != font) {
        calculateCellSize(m_font = font);
        relayout(true);
    }
}

void
FileListing::resizeEvent(QResizeEvent *event)
{
    relayout(false);
}

void
FileListing::paintEvent(QPaintEvent *event)
{
    QRegion region = visibleRegion();
    QPainter painter(this);
    painter.setFont(m_font);

    for (int i = 0, n = m_list.size(); i < n; ++i)
        if (region.intersects(m_list.at(i).second))
        {
            QStyle::State state = 0;

            if (m_selected == i)
                state |= QStyle::State_Selected;
            if (m_mouseover == i)
                state |= QStyle::State_MouseOver;

            auto index = m_filter->index(i, FILE_COLUMN_NAME);
            m_nameitem->drawName(&painter, m_list.at(i).second, state, index);
        }
}

void
FileListing::leaveEvent(QEvent *event)
{
    if (m_mouseover != -1) {
        m_mouseover = -1;
        update();
    }
}

void
FileListing::showEvent(QShowEvent *event)
{
    m_visible = true;
    relayout(m_restart);
}

void
FileListing::hideEvent(QHideEvent *event)
{
    m_visible = false;
    m_restart = false;
}

inline int
FileListing::getClickIndex(QPoint pos) const
{
    for (int i = 0, n = m_list.size(); i < n; ++i)
        if (m_list[i].second.contains(pos))
            return i;

    return -1;
}

inline void
FileListing::mouseAction(int action) const
{
    m_parent->action(action);
}

void
FileListing::mousePressEvent(QMouseEvent *event)
{
    m_nameitem->setBlinkEffect();

    int selected = getClickIndex(event->pos());
    if (selected != m_parent->selectedIndex()) {
        m_selected = selected;
        m_parent->updateSelection(selected);
        update();
    }

    m_dragStartPosition = event->pos();
    m_clicking = true;
    event->accept();
}

void
FileListing::mouseMoveEvent(QMouseEvent *event)
{
    int dist = (event->pos() - m_dragStartPosition).manhattanLength();
    bool isdrag = dist >= QApplication::startDragDistance();

    if (event->buttons()) {
        if (isdrag && (event->buttons() & Qt::LeftButton) && m_selected != -1)
            m_parent->startDrag(m_parent->selectedUrl().path());
    }
    else if (m_mouseover < 0 || m_mouseover >= m_list.size() ||
             !m_list.at(m_mouseover).second.contains(event->pos()))
    {
        int mouseover = getClickIndex(event->pos());
        if (m_mouseover != mouseover) {
            m_mouseover = mouseover;
            update();
        }
    }

    if (m_clicking && isdrag) {
        m_clicking = false;
    }

    event->accept();
}

void
FileListing::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clicking) {
        m_clicking = false;

        switch (event->button()) {
        case Qt::LeftButton:
            if (event->modifiers() == Qt::ControlModifier)
                mouseAction(g_global->fileAction1());
            else if (event->modifiers() == Qt::ShiftModifier)
                mouseAction(g_global->fileAction2());
            break;
        case Qt::MiddleButton:
            if (event->modifiers() == 0)
                mouseAction(g_global->fileAction3());
            break;
        default:
            break;
        }
    }

    event->accept();
}

void
FileListing::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->modifiers() == 0)
        mouseAction(g_global->fileAction0());

    event->accept();
}

void
FileListing::contextMenuEvent(QContextMenuEvent *event)
{
    int idx = getClickIndex(event->pos());
    auto *file = (idx != -1) ? FILE_FILEP(m_filter->index(idx, 0)) : nullptr;

    m_parent->getFilePopup(file)->popup(event->globalPos());
    event->accept();
}

void
FileListing::selectItem(int selected)
{
    m_selected = selected;
    update();
    if (selected >= 0) {
        const auto &rect = m_list.at(selected).second;
        QPoint p = rect.center();
        m_scroll->ensureVisible(p.x(), p.y(), rect.width() / 2, rect.height());
    }
}

QSize
FileListing::sizeHint() const
{
    return m_sizeHint;
}
