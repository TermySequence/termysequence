// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "manager.h"
#include "stack.h"
#include "mainwindow.h"
#include "dockwidget.h"
#include "jobwidget.h"
#include "notewidget.h"
#include "taskwidget.h"
#include "filewidget.h"

TermInstance *
TermManager::lastToolTerm()
{
    auto *lastTool = m_parent->lastTool();
    return lastTool ?
        static_cast<ToolWidget*>(lastTool->widget())->selectedTerm() :
        nullptr;
}

ServerInstance *
TermManager::lastToolServer()
{
    auto *lastTool = m_parent->lastTool();
    return lastTool ?
        static_cast<ToolWidget*>(lastTool->widget())->selectedServer() :
        nullptr;
}

regionid_t
TermManager::lastToolRegion()
{
    auto *lastTool = m_parent->lastTool();
    return lastTool ?
        static_cast<ToolWidget*>(lastTool->widget())->selectedRegion() :
        INVALID_REGION_ID;
}

void
TermManager::actionToggleTerminalsTool()
{
    m_parent->terminalsDock()->bringToggle();
}

void
TermManager::actionToggleKeymapTool()
{
    m_parent->keymapDock()->bringToggle();
}

void
TermManager::actionToggleSuggestionsTool()
{
    m_parent->commandsDock()->bringToggle();
}

void
TermManager::actionToggleSearchTool()
{
    m_parent->searchDock()->bringToggle();
}

void
TermManager::actionToggleHistoryTool()
{
    m_parent->jobsDock()->bringToggle();
}

void
TermManager::actionToggleAnnotationsTool()
{
    m_parent->notesDock()->bringToggle();
}

void
TermManager::actionToggleTasksTool()
{
    m_parent->tasksDock()->bringToggle();
}

void
TermManager::actionToggleFilesTool()
{
    m_parent->filesDock()->bringToggle();
}

void
TermManager::actionRaiseTerminalsTool()
{
    m_parent->terminalsDock()->bringUp();
}

void
TermManager::actionRaiseKeymapTool()
{
    m_parent->keymapDock()->bringUp();
}

void
TermManager::actionRaiseSuggestionsTool()
{
    m_parent->commandsDock()->bringUp();
}

void
TermManager::actionRaiseSearchTool()
{
    m_parent->searchDock()->bringUp();
}

void
TermManager::actionRaiseHistoryTool()
{
    m_parent->jobsDock()->bringUp();
}

void
TermManager::actionRaiseAnnotationsTool()
{
    m_parent->notesDock()->bringUp();
}

void
TermManager::actionRaiseTasksTool()
{
    m_parent->tasksDock()->bringUp();
}

void
TermManager::actionRaiseFilesTool()
{
    m_parent->filesDock()->bringUp();
}

void
TermManager::actionRaiseActiveTool()
{
    m_parent->lastTool()->bringUp();
}

void
TermManager::actionHistoryFirst()
{
    m_parent->jobsDock()->bringUp();
    m_parent->jobsWidget()->selectFirst();
}

void
TermManager::actionHistoryPrevious()
{
    m_parent->jobsDock()->bringUp();
    m_parent->jobsWidget()->selectPrevious();
}

void
TermManager::actionHistoryNext()
{
    m_parent->jobsDock()->bringUp();
    m_parent->jobsWidget()->selectNext();
}

void
TermManager::actionHistoryLast()
{
    m_parent->jobsDock()->bringUp();
    m_parent->jobsWidget()->selectLast();
}

void
TermManager::actionHistorySearch()
{
    m_parent->jobsDock()->bringUp();
    m_parent->jobsWidget()->takeFocus(m_stack ? m_stack->pos() : 0);
}

void
TermManager::actionHistorySearchReset()
{
    m_parent->jobsWidget()->resetSearch();
}

void
TermManager::actionNoteFirst()
{
    m_parent->notesDock()->bringUp();
    m_parent->notesWidget()->selectFirst();
}

void
TermManager::actionNotePrevious()
{
    m_parent->notesDock()->bringUp();
    m_parent->notesWidget()->selectPrevious();
}

void
TermManager::actionNoteNext()
{
    m_parent->notesDock()->bringUp();
    m_parent->notesWidget()->selectNext();
}

void
TermManager::actionNoteLast()
{
    m_parent->notesDock()->bringUp();
    m_parent->notesWidget()->selectLast();
}

void
TermManager::actionNoteSearch()
{
    m_parent->notesDock()->bringUp();
    m_parent->notesWidget()->takeFocus(m_stack ? m_stack->pos() : 0);
}

void
TermManager::actionNoteSearchReset()
{
    m_parent->notesWidget()->resetSearch();
}

void
TermManager::actionTaskFirst()
{
    m_parent->tasksDock()->bringUp();
    m_parent->tasksWidget()->selectFirst();
}

void
TermManager::actionTaskPrevious()
{
    m_parent->tasksDock()->bringUp();
    m_parent->tasksWidget()->selectPrevious();
}

void
TermManager::actionTaskNext()
{
    m_parent->tasksDock()->bringUp();
    m_parent->tasksWidget()->selectNext();
}

void
TermManager::actionTaskLast()
{
    m_parent->tasksDock()->bringUp();
    m_parent->tasksWidget()->selectLast();
}

void
TermManager::actionFileFirst()
{
    m_parent->filesDock()->bringUp();
    m_parent->filesWidget()->selectFirst();
}

void
TermManager::actionFilePrevious()
{
    m_parent->filesDock()->bringUp();
    m_parent->filesWidget()->selectPrevious();
}

void
TermManager::actionFileNext()
{
    m_parent->filesDock()->bringUp();
    m_parent->filesWidget()->selectNext();
}

void
TermManager::actionFileLast()
{
    m_parent->filesDock()->bringUp();
    m_parent->filesWidget()->selectLast();
}

void
TermManager::actionFileSearch()
{
    m_parent->filesDock()->bringUp();
    m_parent->filesWidget()->takeFocus(m_stack ? m_stack->pos() : 0);
}

void
TermManager::actionFileSearchReset()
{
    m_parent->filesWidget()->resetSearch();
}

void
TermManager::actionToolFirst()
{
    auto lastTool = m_parent->lastTool();
    static_cast<ToolWidget*>(lastTool->widget())->selectFirst();
    lastTool->bringUp();
}

void
TermManager::actionToolPrevious()
{
    auto lastTool = m_parent->lastTool();
    static_cast<ToolWidget*>(lastTool->widget())->selectPrevious();
    lastTool->bringUp();
}

void
TermManager::actionToolNext()
{
    auto lastTool = m_parent->lastTool();
    static_cast<ToolWidget*>(lastTool->widget())->selectNext();
    lastTool->bringUp();
}

void
TermManager::actionToolLast()
{
    auto lastTool = m_parent->lastTool();
    static_cast<ToolWidget*>(lastTool->widget())->selectLast();
    lastTool->bringUp();
}

void
TermManager::actionToolSearch()
{
    auto lastSearchable = m_parent->lastSearchable();
    static_cast<SearchableWidget*>(lastSearchable->widget())->takeFocus(
        m_stack ? m_stack->pos() : 0);

    lastSearchable->bringUp();
}

void
TermManager::actionToolSearchReset()
{
    auto lastSearchable = m_parent->lastSearchable();
    static_cast<SearchableWidget*>(lastSearchable->widget())->resetSearch();
    lastSearchable->bringUp();
}

void
TermManager::actionToolAction(QString index)
{
    auto lastTool = m_parent->lastTool();
    static_cast<ToolWidget*>(lastTool->widget())->toolAction(index.toInt());
    lastTool->bringUp();
}

void
TermManager::actionToolContextMenu()
{
    auto lastTool = m_parent->lastTool();
    lastTool->bringUp();

    static_cast<ToolWidget*>(lastTool->widget())->contextMenu();
}

void
TermManager::actionToggleToolSearchBar()
{
    auto lastSearchable = m_parent->lastSearchable();
    if (static_cast<SearchableWidget*>(lastSearchable->widget())->toggleSearch())
        actionToolSearch();
}

void
TermManager::actionToggleToolTableHeader()
{
    auto lastSearchable = m_parent->lastSearchable();
    static_cast<SearchableWidget*>(lastSearchable->widget())->toggleHeader();
}
