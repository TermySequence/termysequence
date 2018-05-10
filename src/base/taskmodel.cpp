// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/simpleitem.h"
#include "taskmodel.h"
#include "task.h"
#include "manager.h"
#include "term.h"
#include "dockwidget.h"
#include "mainwindow.h"
#include "taskstatus.h"
#include "listener.h"
#include "infoanim.h"
#include "settings/global.h"
#include "settings/state.h"

#include <QHeaderView>
#include <QScrollBar>
#include <QMenu>
#include <QContextMenuEvent>
#include <QApplication>

#define MODEL_VERSION 1

//
// Model
//
TaskModel::TaskModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    connect(g_global, SIGNAL(settingsLoaded()), SLOT(relimit()));
    m_limit = g_global->taskLimit();
}

void
TaskModel::setActiveTask(TermTask *task)
{
    m_activeTasks.append(task);

    emit activeTaskChanged(m_activeTask = task);
}

void
TaskModel::removeTask(TermTask *task, bool destroy)
{
    m_activeTasks.removeOne(task);

    if (m_activeTask == task) {
        m_activeTask = !m_activeTasks.isEmpty() ? m_activeTasks.back() : nullptr;
        emit activeTaskChanged(m_activeTask);
    }

    if (destroy) {
        m_taskMap.remove(task->id());
        emit taskRemoved(task);
        task->deleteLater();
    }
}

void
TaskModel::relimit()
{
    size_t limit = g_global->taskLimit();

    if (m_limit != limit) {
        if (m_limit > limit) {
            beginResetModel();
            while (m_tasks.size() > limit) {
                removeTask(m_tasks.back(), true);
                m_tasks.pop_front();
            }
            endResetModel();
        }
        m_limit = limit;
    }
}

void
TaskModel::addTask(TermTask *task, TermManager *manager)
{
    connect(task->animation(), SIGNAL(animationSignal(intptr_t)),
            SLOT(handleTaskAnimation(intptr_t)));

    MainWindow *window = manager->parent();

    if (!task->finished()) {
        connect(task, SIGNAL(taskChanged()), SLOT(handleTaskChanged()));
        connect(task, SIGNAL(taskQuestion()), SLOT(handleTaskQuestion()));

        if (!task->longRunning())
            setActiveTask(task);

        if (g_global->autoShowTask() && window && !task->noStatusPopup())
            (new TaskStatus(task, window))->show();
    }

    beginInsertRows(QModelIndex(), 0, 0);
    m_tasks.prepend(task);
    m_taskMap.insert(task->id(), task);
    endInsertRows();
    emit taskAdded(task);

    if (m_tasks.size() > m_limit) {
        int row = m_tasks.size() - 1;
        beginRemoveRows(QModelIndex(), row, row);
        removeTask(m_tasks.back(), true);
        m_tasks.pop_back();
        endRemoveRows();
    }
}

void
TaskModel::handleTaskChanged()
{
    TermTask *task = static_cast<TermTask*>(sender());

    if (task->finished()) {
        task->disconnect(this);
        removeTask(task, false);
        emit taskFinished(task);
    }

    for (int row = 0; row < m_tasks.size(); ++row)
        if (m_tasks[row] == task) {
            QModelIndex start = createIndex(row, 0);
            QModelIndex end = createIndex(row, TASK_N_COLUMNS - 1);
            emit dataChanged(start, end);
            return;
        }
}

void
TaskModel::handleTaskQuestion()
{
    TermTask *task = static_cast<TermTask*>(sender());

    if (!task->dialog()) {
        TermManager *manager = g_listener->activeManager();
        if (manager)
            new TaskStatus(task, manager->parent());
    }
}

void
TaskModel::handleTaskAnimation(intptr_t data)
{
    TermTask *task = reinterpret_cast<TermTask*>(data);

    for (int row = 0; row < m_tasks.size(); ++row)
        if (m_tasks[row] == task) {
            QModelIndex start = createIndex(row, 0);
            QModelIndex end = createIndex(row, TASK_N_COLUMNS - 1);
            emit dataChanged(start, end, QVector<int>(1, Qt::BackgroundRole));
            return;
        }
}

void
TaskModel::removeClosedTerminals()
{
    int row = 0;

    for (auto i = m_tasks.begin(); i != m_tasks.end(); )
    {
        if ((*i)->target() == nullptr) {
            beginRemoveRows(QModelIndex(), row, row);
            removeTask(*i, true);
            i = m_tasks.erase(i);
            endRemoveRows();
        } else {
            ++i;
            ++row;
        }
    }
}

void
TaskModel::removeClosedTasks()
{
    int row = 0;

    for (auto i = m_tasks.begin(); i != m_tasks.end(); )
    {
        if ((*i)->finished()) {
            beginRemoveRows(QModelIndex(), row, row);
            removeTask(*i, true);
            i = m_tasks.erase(i);
            endRemoveRows();
        } else {
            ++i;
            ++row;
        }
    }
}

/*
 * Model functions
 */
int
TaskModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : TASK_N_COLUMNS;
}

int
TaskModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_tasks.size();
}

bool
TaskModel::hasChildren(const QModelIndex &parent) const
{
    return !parent.isValid() && !m_tasks.empty();
}

bool
TaskModel::hasIndex(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && row < m_tasks.size() && column < TASK_N_COLUMNS;
}

QModelIndex
TaskModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid() && row < m_tasks.size())
        return createIndex(row, column, (void *)m_tasks[row]);
    else
        return QModelIndex();
}

QVariant
TaskModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch (section) {
        case TASK_COLUMN_PROGRESS:
            return tr("Progress", "heading");
        case TASK_COLUMN_TYPE:
            return tr("Type", "heading");
        case TASK_COLUMN_FROM:
            return tr("From", "heading");
        case TASK_COLUMN_SOURCE:
            return tr("Source", "heading");
        case TASK_COLUMN_TO:
            return tr("To", "heading");
        case TASK_COLUMN_SINK:
            return tr("Destination", "heading");
        case TASK_COLUMN_SENT:
            return tr("Sent", "heading");
        case TASK_COLUMN_RECEIVED:
            return tr("Received", "heading");
        case TASK_COLUMN_STARTED:
            return tr("Started", "heading");
        case TASK_COLUMN_FINISHED:
            return tr("Finished", "heading");
        case TASK_COLUMN_STATUS:
            return tr("Status", "heading");
        case TASK_COLUMN_ID:
            return tr("Identifier", "heading");
        default:
            break;
        }

    return QVariant();
}

QVariant
TaskModel::data(const QModelIndex &index, int role) const
{
    const auto *task = (const TermTask *)index.internalPointer();
    if (task)
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case TASK_COLUMN_PROGRESS:
                if (task->finished())
                    return C(task->succeeded() ? 0x2714 : 0x2718);
                break;
            case TASK_COLUMN_TYPE:
                return task->typeStr();
            case TASK_COLUMN_FROM:
                return task->fromStr();
            case TASK_COLUMN_SOURCE:
                return task->sourceStr();
            case TASK_COLUMN_TO:
                return task->toStr();
            case TASK_COLUMN_SINK:
                return task->sinkStr();
            case TASK_COLUMN_SENT:
                return QString::number(task->sent());
            case TASK_COLUMN_RECEIVED:
                return QString::number(task->received());
            case TASK_COLUMN_STARTED:
                return task->startedStr();
            case TASK_COLUMN_FINISHED:
                return task->finishedStr();
            case TASK_COLUMN_STATUS:
                return task->statusStr();
            case TASK_COLUMN_ID:
                return task->idStr();
            }
            break;
        case Qt::DecorationRole:
            switch (index.column()) {
            case TASK_COLUMN_TYPE:
                return task->typeIcon();
            }
            break;
        case Qt::TextAlignmentRole:
            switch (index.column()) {
            case TASK_COLUMN_PROGRESS:
                return Qt::AlignCenter;
            }
            break;
        case Qt::BackgroundRole:
            return task->animation()->colorVariant();
        case Qt::ForegroundRole:
            switch (index.column()) {
            case TASK_COLUMN_PROGRESS:
                return g_global->color(task->finishColor());
            }
            break;
        case Qt::UserRole:
            return task->progress();
        case TASK_ROLE_TASK:
            return QVariant::fromValue((QObject *)task);
        }

    return QVariant();
}

Qt::ItemFlags
TaskModel::flags(const QModelIndex &) const
{
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable|
        Qt::ItemNeverHasChildren|Qt::ItemIsDragEnabled;
}

QStringList
TaskModel::mimeTypes() const
{
    auto result = QAbstractItemModel::mimeTypes();
    result.append(A("text/plain"));
    result.append(A("text/uri-list"));
    return result;
}

QMimeData *
TaskModel::mimeData(const QModelIndexList &indexes) const
{
    auto *result = QAbstractItemModel::mimeData(indexes);

    if (!indexes.isEmpty()) {
        auto task = (const TermTask *)indexes.at(0).internalPointer();
        if (task)
            task->getDragData(result);
    }

    return result;
}

bool
TaskModel::dropMimeData(const QMimeData*, Qt::DropAction, int, int, const QModelIndex&)
{
    return false;
}

//
// Filter
//
TaskFilter::TaskFilter(TaskModel *model, QObject *parent) :
    QSortFilterProxyModel(parent),
    m_model(model)
{
    setDynamicSortFilter(false);
    setSourceModel(model);
}

void
TaskFilter::setWhitelist(const Tsq::Uuid &id)
{
    m_whitelist.clear();
    m_whitelist.insert(id);
    m_haveWhitelist = true;
    invalidateFilter();
}

void
TaskFilter::addWhitelist(const Tsq::Uuid &id)
{
    m_whitelist.insert(id);
    m_haveWhitelist = true;
    invalidateFilter();
}

void
TaskFilter::emptyWhitelist()
{
    m_whitelist.clear();
    m_haveWhitelist = true;
    invalidateFilter();
}

void
TaskFilter::addBlacklist(const Tsq::Uuid &id)
{
    m_blacklist.insert(id);
    m_haveBlacklist = true;
    invalidateFilter();
}

void
TaskFilter::resetFilter()
{
    m_whitelist.clear();
    m_blacklist.clear();
    m_haveWhitelist = m_haveBlacklist = false;
    invalidateFilter();
}

bool
TaskFilter::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = m_model->index(row, 0, parent);
    TermTask *task = (TermTask *)index.internalPointer();

    if (m_haveWhitelist) {
        if (!m_whitelist.contains(task->termId()) && !m_whitelist.contains(task->serverId()))
            return false;
    }
    else if (m_haveBlacklist) {
        if (m_blacklist.contains(task->termId()) || m_blacklist.contains(task->serverId()))
            return false;
    }

    return true;
}

bool
TaskFilter::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto *ltask = (const TermTask *)left.internalPointer();
    const auto *rtask = (const TermTask *)right.internalPointer();

    switch (left.column()) {
    case TASK_COLUMN_STARTED:
        if (ltask->startedDate() != rtask->startedDate())
            return ltask->startedDate() < rtask->startedDate();
        break;
    case TASK_COLUMN_FINISHED:
        if (ltask->finishedDate() != rtask->finishedDate())
            return ltask->finishedDate() < rtask->finishedDate();
        break;
    case TASK_COLUMN_PROGRESS:
        break;
    default:
        return QSortFilterProxyModel::lessThan(left, right);
    }

    if (ltask->progress() != rtask->progress())
        return ltask->progress() < rtask->progress();

    if (ltask->startedDate() != rtask->startedDate())
        return ltask->startedDate() < rtask->startedDate();

    return ltask < rtask;
}

//
// View
//

#define OBJPROP_INDEX "viewHeaderIndex"

TaskView::TaskView(TaskFilter *filter, ToolWidget *parent) :
    m_filter(filter),
    m_parent(parent),
    m_window(parent->window()),
    m_headerMenu(nullptr)
{
    QItemSelectionModel *m = selectionModel();
    setModel(filter);
    delete m;

    setFocusPolicy(Qt::NoFocus);

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(TASK_COLUMN_PROGRESS, Qt::DescendingOrder);
    auto *progressItem = new ProgressBarItemDelegate(this);
    setItemDelegateForColumn(TASK_COLUMN_PROGRESS, progressItem);

    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    verticalHeader()->setVisible(false);
    m_header = horizontalHeader();
    m_header->setSectionResizeMode(QHeaderView::Interactive);
    m_header->setSectionsMovable(true);
    m_header->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_window->manager(), SIGNAL(populated()), SLOT(resizeColumnsToContents()));

    connect(filter->sourceModel(),
            SIGNAL(modelReset()),
            SLOT(resizeColumnsToContents()));
    connect(filter->sourceModel(),
            SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&)),
            SLOT(resizeColumnsToContents()));
    connect(filter->sourceModel(),
            SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            SLOT(handleRowAdded()));

    connect(m_header,
            SIGNAL(customContextMenuRequested(const QPoint&)),
            SLOT(handleHeaderContextMenu(const QPoint&)));

    viewport()->installEventFilter(parent);
    m_header->viewport()->installEventFilter(parent);
    verticalScrollBar()->installEventFilter(parent);
    horizontalScrollBar()->installEventFilter(parent);
}

void
TaskView::handleRowAdded()
{
    resizeColumnsToContents();
    scrollTo(m_filter->index(0, 0));
}

void
TaskView::action(int type, TermTask *task)
{
    if (!task)
        return;

    QString tmp;

    switch (type) {
    case TaskActFileDesk:
        tmp = A("OpenTaskFile|") + g_str_DESKTOP_LAUNCH + '|';
        break;
    case TaskActFile:
        tmp = A("OpenTaskFile||");
        break;
    case TaskActDirDesk:
        tmp = A("OpenTaskDirectory|") + g_str_DESKTOP_LAUNCH + '|';
        break;
    case TaskActDir:
        tmp = A("OpenTaskDirectory||");
        break;
    case TaskActTerm:
        tmp = A("OpenTaskTerminal||");
        break;
    case TaskActInspect:
        tmp = A("InspectTask|");
        break;
    case TaskActCancel:
        tmp = A("CancelTask|");
        break;
    case TaskActRestart:
        tmp = A("RestartTask|");
        break;
    default:
        return;
    }

    tmp += task->idStr();
    m_window->manager()->invokeSlot(tmp);
}

inline void
TaskView::mouseAction(int action)
{
    this->action(action, TASK_TASKP(indexAt(m_dragStartPosition)));
}

void
TaskView::mousePressEvent(QMouseEvent *event)
{
    m_dragStartPosition = event->pos();
    m_clicking = true;
    QTableView::mousePressEvent(event);
}

void
TaskView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_clicking) {
        int dist = (event->pos() - m_dragStartPosition).manhattanLength();
        m_clicking = dist <= QApplication::startDragDistance();
    }

    QTableView::mouseMoveEvent(event);
}

void
TaskView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clicking) {
        m_clicking = false;

        switch (event->button()) {
        case Qt::LeftButton:
            if (event->modifiers() == Qt::ControlModifier)
                mouseAction(g_global->taskAction1());
            else if (event->modifiers() == Qt::ShiftModifier)
                mouseAction(g_global->taskAction2());
            break;
        case Qt::MiddleButton:
            if (event->modifiers() == 0)
                mouseAction(g_global->taskAction3());
            break;
        default:
            break;
        }
    }

    QTableView::mouseReleaseEvent(event);
}

void
TaskView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->modifiers() == 0)
        mouseAction(g_global->taskAction0());

    event->accept();
}

void
TaskView::contextMenuEvent(QContextMenuEvent *event)
{
    TermTask *task = TASK_TASKP(indexAt(event->pos()));

    m_window->getTaskPopup(this, task)->popup(event->globalPos());
    event->accept();
}

void
TaskView::contextMenu(QPoint point)
{
    TermTask *task = nullptr;

    auto sel = selectionModel()->selectedRows();
    if (!sel.isEmpty())
        task = TASK_TASKP(sel.front());

    m_window->getTaskPopup(this, task)->popup(point);
}

void
TaskView::handleHeaderContextMenu(const QPoint &pos)
{
    if (!m_headerMenu) {
        m_headerMenu = new QMenu(this);

        for (int i = 0; i < TASK_N_COLUMNS; ++i) {
            QVariant v = m_filter->headerData(i, Qt::Horizontal, Qt::DisplayRole);

            QAction *a = new QAction(v.toString(), m_headerMenu);
            a->setCheckable(true);
            a->setProperty(OBJPROP_INDEX, i + 1);

            connect(a, SIGNAL(triggered(bool)), SLOT(handleHeaderTriggered(bool)));
            m_headerMenu->addAction(a);
        }

        connect(m_headerMenu, SIGNAL(aboutToShow()), SLOT(preshowHeaderContextMenu()));
    }

    m_headerMenu->popup(mapToGlobal(pos));
}

void
TaskView::preshowHeaderContextMenu()
{
    for (auto i: m_headerMenu->actions())
    {
        int idx = i->property(OBJPROP_INDEX).toInt() - 1;

        if (idx >= 0)
            i->setChecked(!m_header->isSectionHidden(idx));
    }
}

void
TaskView::handleHeaderTriggered(bool checked)
{
    int idx = sender()->property(OBJPROP_INDEX).toInt() - 1;

    if (idx >= 0) {
        m_header->setSectionHidden(idx, !checked);

        // Don't permit everything to be hidden
        bool somethingShown = false;

        for (int i = 0; i < TASK_N_COLUMNS; ++i)
            if (!m_header->isSectionHidden(i)) {
                somethingShown = true;
                break;
            }

        if (!somethingShown)
            m_header->setSectionHidden(idx, false);
    }
}

void
TaskView::selectFirst()
{
    if (m_filter->rowCount()) {
        QModelIndex index = m_filter->index(0, 0);
        selectRow(index.row());
        scrollTo(index);
    }
}

void
TaskView::selectPrevious()
{
    auto sel = selectionModel()->selectedRows();
    int row;

    if (sel.isEmpty()) {
        selectLast();
    } else if ((row = sel.front().row())) {
        QModelIndex index = m_filter->index(row - 1, 0);
        selectRow(index.row());
        scrollTo(index);
    }
}

void
TaskView::selectNext()
{
    auto sel = selectionModel()->selectedRows();
    int row;

    if (sel.isEmpty()) {
        selectFirst();
    } else if ((row = sel.front().row()) < m_filter->rowCount() - 1) {
        QModelIndex index = m_filter->index(row + 1, 0);
        selectRow(index.row());
        scrollTo(index);
    }
}

void
TaskView::selectLast()
{
    int rows = m_filter->rowCount();
    if (rows) {
        QModelIndex index = m_filter->index(rows - 1, 0);
        selectRow(index.row());
        scrollTo(index);
    }
}

void
TaskView::restoreState(int index)
{
    // Restore table headers
    auto bytes = g_state->fetchVersioned(TasksColumnsKey, MODEL_VERSION, index);
    if (bytes.isEmpty() || !m_header->restoreState(bytes))
    {
        QStringList headers(DEFAULT_TASKVIEW_COLUMNS);

        for (int i = 0; i < TASK_N_COLUMNS; ++i) {
            bool shown = headers.contains(QString::number(i));
            m_header->setSectionHidden(i, !shown);
        }
    }
}

void
TaskView::saveState(int index)
{
    // Save table headers
    auto bytes = m_header->saveState();
    g_state->storeVersioned(TasksColumnsKey, MODEL_VERSION, bytes, index);
}
