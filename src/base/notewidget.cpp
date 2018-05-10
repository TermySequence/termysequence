// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/icons.h"
#include "notewidget.h"
#include "notemodel.h"
#include "manager.h"
#include "term.h"
#include "settings/global.h"

#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>

#define TR_CHECK1 TL("input-checkbox", "Autoscroll")

NoteWidget::NoteWidget(TermManager *manager, NoteModel *model) :
    SearchableWidget(manager)
{
    m_filter = new NoteFilter(model, this);
    m_view = new NoteView(m_filter, this);

    m_check = new QCheckBox(TR_CHECK1);
    m_check->installEventFilter(this);

    auto *button = new QPushButton;
    button->setIcon(QI(ICON_CLOSE_SEARCH));
    button->installEventFilter(this);

    QHBoxLayout *lineLayout = new QHBoxLayout;
    lineLayout->setContentsMargins(g_mtmargins);
    lineLayout->setSpacing(10);
    lineLayout->addWidget(m_check);
    lineLayout->addWidget(m_line, 1);
    lineLayout->addWidget(button);
    m_bar = new QWidget;
    m_bar->setLayout(lineLayout);
    m_header = m_view->horizontalHeader();

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(g_mtmargins);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_view, 1);
    mainLayout->addWidget(m_bar);
    setLayout(mainLayout);

    connect(m_line, SIGNAL(textChanged(const QString&)),
            m_filter, SLOT(setSearchString(const QString&)));

    connect(m_check, SIGNAL(toggled(bool)),
            m_view, SLOT(setAutoscroll(bool)));

    connect(button, SIGNAL(clicked()),
            manager, SLOT(actionToggleToolSearchBar()));
}

const TermNote *
NoteWidget::selectedNote() const
{
    auto sel = m_view->selectionModel()->selectedRows();
    return !sel.isEmpty() ? NOTE_NOTEP(sel.front()) : nullptr;
}

TermInstance *
NoteWidget::selectedTerm() const
{
    auto sel = m_view->selectionModel()->selectedRows();
    return !sel.isEmpty() ? NOTE_TERMP(sel.front()) : nullptr;
}

regionid_t
NoteWidget::selectedRegion() const
{
    auto sel = m_view->selectionModel()->selectedRows();
    return !sel.isEmpty() ?
        sel.front().data(NOTE_ROLE_REGION).toUInt() :
        INVALID_REGION_ID;
}

void
NoteWidget::toolAction(int index)
{
    int action;

    switch (index) {
    case 0:
        action = g_global->noteAction0();
        break;
    case 1:
        action = g_global->noteAction1();
        break;
    case 2:
        action = g_global->noteAction2();
        break;
    case 3:
        action = g_global->noteAction3();
        break;
    default:
        return;
    }

    m_view->action(action, selectedNote());
}

void
NoteWidget::selectFirst()
{
    m_view->selectFirst();
}

void
NoteWidget::selectPrevious()
{
    m_view->selectPrevious();
}

void
NoteWidget::selectNext()
{
    m_view->selectNext();
}

void
NoteWidget::selectLast()
{
    m_view->selectLast();
}

void
NoteWidget::restoreState(int index)
{
    // Returns bar, header, and autoscroll settings
    bool setting[3] = { true, true, false };
    m_view->restoreState(index, setting);

    m_bar->setVisible(m_barShown = setting[0]);
    m_header->setVisible(m_headerShown = setting[1]);
    m_check->setChecked(setting[2]);
}

void
NoteWidget::saveState(int index)
{
    bool setting[2] = { m_barShown, m_headerShown };
    m_view->saveState(index, setting);
}

void
NoteWidget::setWhitelist(const Tsq::Uuid &id)
{
    m_filter->setWhitelist(id);
}

void
NoteWidget::addWhitelist(const Tsq::Uuid &id)
{
    m_filter->addWhitelist(id);
}

void
NoteWidget::emptyWhitelist()
{
    m_filter->emptyWhitelist();
}

void
NoteWidget::addBlacklist(const Tsq::Uuid &id)
{
    m_filter->addBlacklist(id);
}

void
NoteWidget::resetFilter()
{
    m_filter->resetFilter();
}

void
NoteWidget::removeClosedTerminals()
{
    m_filter->model()->removeClosedTerminals();
}

void
NoteWidget::contextMenu()
{
    QPoint mid(width() / 2, height() / 4);
    m_view->contextMenu(mapToGlobal(mid));
}
