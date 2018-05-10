// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/uuid.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QHash>

#define TASK_COLUMN_PROGRESS    0
#define TASK_COLUMN_TYPE        1
#define TASK_COLUMN_FROM        2
#define TASK_COLUMN_SOURCE      3
#define TASK_COLUMN_TO          4
#define TASK_COLUMN_SINK        5
#define TASK_COLUMN_SENT        6
#define TASK_COLUMN_RECEIVED    7
#define TASK_COLUMN_STARTED     8
#define TASK_COLUMN_FINISHED    9
#define TASK_COLUMN_STATUS      10
#define TASK_COLUMN_ID          11
#define TASK_N_COLUMNS          12

#define TASK_ROLE_TASK      Qt::ItemDataRole(Qt::UserRole + 1)

#define TASK_TASKP(i) \
    static_cast<TermTask*>(i.data(TASK_ROLE_TASK).value<QObject*>())

QT_BEGIN_NAMESPACE
class QHeaderView;
class QMenu;
QT_END_NAMESPACE
class TermTask;
class TermManager;
class ToolWidget;
class MainWindow;

//
// Model
//
class TaskModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    QList<TermTask*> m_tasks;
    QHash<Tsq::Uuid,TermTask*> m_taskMap;

    size_t m_limit;

    TermTask *m_activeTask = nullptr;
    QList<TermTask*> m_activeTasks;

private slots:
    void handleTaskChanged();
    void handleTaskQuestion();
    void handleTaskAnimation(intptr_t data);

    void relimit();
    void setActiveTask(TermTask *task);
    void removeTask(TermTask *task, bool destroy);

signals:
    void taskAdded(TermTask *task);
    void taskFinished(TermTask *task);
    void taskRemoved(TermTask *task);
    void activeTaskChanged(TermTask *task);

public:
    TaskModel(QObject *parent);

    inline const auto& tasks() const { return m_tasks; }
    inline TermTask* activeTask() const { return m_activeTask; }
    inline TermTask* getTask(const Tsq::Uuid &id) const { return m_taskMap.value(id); }

    void addTask(TermTask *task, TermManager *manager);
    void invalidateTasks(int64_t started);
    void removeClosedTerminals();
    void removeClosedTasks();

public:
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    bool hasChildren(const QModelIndex &parent) const;
    bool hasIndex(int row, int column, const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QStringList mimeTypes() const;
    QMimeData* mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
};

//
// Filter
//
class TaskFilter final: public QSortFilterProxyModel
{
    Q_OBJECT

private:
    TaskModel *m_model;

    QSet<Tsq::Uuid> m_whitelist;
    QSet<Tsq::Uuid> m_blacklist;
    bool m_haveWhitelist = false;
    bool m_haveBlacklist = false;

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

public:
    TaskFilter(TaskModel *model, QObject *parent);

    inline TaskModel* model() { return m_model; }

    void setWhitelist(const Tsq::Uuid &id);
    void addWhitelist(const Tsq::Uuid &id);
    void emptyWhitelist();
    void addBlacklist(const Tsq::Uuid &id);
    void resetFilter();
};

//
// View
//
class TaskView final: public QTableView
{
    Q_OBJECT

private:
    TaskFilter *m_filter;
    ToolWidget *m_parent;
    MainWindow *m_window;
    QHeaderView *m_header;
    QMenu *m_headerMenu;

    QPoint m_dragStartPosition;
    bool m_clicking = false;
    void mouseAction(int action);

private slots:
    void handleHeaderContextMenu(const QPoint &pos);
    void preshowHeaderContextMenu();
    void handleHeaderTriggered(bool checked);
    void handleRowAdded();

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    TaskView(TaskFilter *filter, ToolWidget *parent);

    void action(int type, TermTask *task);
    void contextMenu(QPoint point);

    void selectFirst();
    void selectPrevious();
    void selectNext();
    void selectLast();

    void restoreState(int index);
    void saveState(int index);
};
