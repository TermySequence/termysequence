// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "dockwidget.h"
#include "lib/uuid.h"

class TermTask;
class TaskModel;
class TaskFilter;
class TaskView;

class TaskWidget final: public ToolWidget
{
private:
    TaskFilter *m_filter;
    TaskView *m_view;

public:
    TaskWidget(TermManager *manager, TaskModel *model);

    TermTask* selectedTask() const;
    bool selectedTaskCancelable() const;
    bool selectedTaskClonable() const;
    QString selectedTaskFile() const;
    TermInstance* selectedTerm() const;
    ServerInstance* selectedServer() const;

    void toolAction(int index);
    void contextMenu();

    void selectFirst();
    void selectPrevious();
    void selectNext();
    void selectLast();

    void setWhitelist(const Tsq::Uuid &id);
    void addWhitelist(const Tsq::Uuid &id);
    void emptyWhitelist();
    void addBlacklist(const Tsq::Uuid &id);
    void resetFilter();

    void removeClosedTerminals();
    void removeClosedTasks();

    void restoreState(int index);
    void saveState(int index);
};
