// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "taskwidget.h"
#include "taskmodel.h"
#include "task.h"
#include "manager.h"
#include "settings/global.h"

#include <QVBoxLayout>
#include <QScrollBar>

TaskWidget::TaskWidget(TermManager *manager, TaskModel *model) :
    ToolWidget(manager)
{
    m_filter = new TaskFilter(model, this);
    m_view = new TaskView(m_filter, this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(g_mtmargins);
    mainLayout->addWidget(m_view, 1);
    setLayout(mainLayout);
}

TermTask *
TaskWidget::selectedTask() const
{
    auto sel = m_view->selectionModel()->selectedRows();
    return !sel.isEmpty() ? TASK_TASKP(sel.front()) : nullptr;
}

bool
TaskWidget::selectedTaskCancelable() const
{
    auto task = selectedTask();
    return task ? !task->finished() : false;
}

bool
TaskWidget::selectedTaskClonable() const
{
    auto task = selectedTask();
    return task ? task->clonable() : false;
}

QString
TaskWidget::selectedTaskFile() const
{
    auto task = selectedTask();
    return task ? task->launchfile() : g_mtstr;
}

TermInstance *
TaskWidget::selectedTerm() const
{
    auto *task = selectedTask();
    return task ? task->term() : nullptr;
}

ServerInstance *
TaskWidget::selectedServer() const
{
    auto *task = selectedTask();
    return task ? task->server() : nullptr;
}

void
TaskWidget::selectFirst()
{
    m_view->selectFirst();
}

void
TaskWidget::selectPrevious()
{
    m_view->selectPrevious();
}

void
TaskWidget::selectNext()
{
    m_view->selectNext();
}

void
TaskWidget::selectLast()
{
    m_view->selectLast();
}

void
TaskWidget::restoreState(int index)
{
    m_view->restoreState(index);
}

void
TaskWidget::saveState(int index)
{
    m_view->saveState(index);
}

void
TaskWidget::setWhitelist(const Tsq::Uuid &id)
{
    m_filter->setWhitelist(id);
}

void
TaskWidget::addWhitelist(const Tsq::Uuid &id)
{
    m_filter->addWhitelist(id);
}

void
TaskWidget::emptyWhitelist()
{
    m_filter->emptyWhitelist();
}

void
TaskWidget::addBlacklist(const Tsq::Uuid &id)
{
    m_filter->addBlacklist(id);
}

void
TaskWidget::resetFilter()
{
    m_filter->resetFilter();
}

void
TaskWidget::removeClosedTerminals()
{
    m_filter->model()->removeClosedTerminals();
}

void
TaskWidget::removeClosedTasks()
{
    m_filter->model()->removeClosedTasks();
}

void
TaskWidget::toolAction(int index)
{
    int action;

    switch (index) {
    case 0:
        action = g_global->taskAction0();
        break;
    case 1:
        action = g_global->taskAction1();
        break;
    case 2:
        action = g_global->taskAction2();
        break;
    case 3:
        action = g_global->taskAction3();
        break;
    default:
        return;
    }

    m_view->action(action, selectedTask());
}

void
TaskWidget::contextMenu()
{
    QPoint mid(width() / 2, height() / 4);
    m_view->contextMenu(mapToGlobal(mid));
}
