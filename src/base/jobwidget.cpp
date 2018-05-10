// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/icons.h"
#include "jobwidget.h"
#include "jobmodel.h"
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

JobWidget::JobWidget(TermManager *manager, JobModel *model) :
    SearchableWidget(manager)
{
    m_filter = new JobFilter(model, this);
    m_view = new JobView(m_filter, this);

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

const TermJob *
JobWidget::selectedJob() const
{
    auto sel = m_view->selectionModel()->selectedRows();
    return !sel.isEmpty() ? JOB_JOBP(sel.front()) : nullptr;
}

TermInstance *
JobWidget::selectedTerm() const
{
    auto sel = m_view->selectionModel()->selectedRows();
    return !sel.isEmpty() ? JOB_TERMP(sel.front()) : nullptr;
}

regionid_t
JobWidget::selectedRegion() const
{
    auto sel = m_view->selectionModel()->selectedRows();
    return !sel.isEmpty() ?
        sel.front().data(JOB_ROLE_REGION).toUInt() :
        INVALID_REGION_ID;
}

void
JobWidget::toolAction(int index)
{
    int action;

    switch (index) {
    case 0:
        action = g_global->jobAction0();
        break;
    case 1:
        action = g_global->jobAction1();
        break;
    case 2:
        action = g_global->jobAction2();
        break;
    case 3:
        action = g_global->jobAction3();
        break;
    default:
        return;
    }

    m_view->action(action, selectedJob());
}

void
JobWidget::selectFirst()
{
    m_view->selectFirst();
}

void
JobWidget::selectPrevious()
{
    m_view->selectPrevious();
}

void
JobWidget::selectNext()
{
    m_view->selectNext();
}

void
JobWidget::selectLast()
{
    m_view->selectLast();
}

void
JobWidget::restoreState(int index)
{
    // Returns bar, header, and autoscroll settings
    bool setting[3] = { true, true, false };
    m_view->restoreState(index, setting);

    m_bar->setVisible(m_barShown = setting[0]);
    m_header->setVisible(m_headerShown = setting[1]);
    m_check->setChecked(setting[2]);
}

void
JobWidget::saveState(int index)
{
    bool setting[2] = { m_barShown, m_headerShown };
    m_view->saveState(index, setting);
}

void
JobWidget::setWhitelist(const Tsq::Uuid &id)
{
    m_filter->setWhitelist(id);
}

void
JobWidget::addWhitelist(const Tsq::Uuid &id)
{
    m_filter->addWhitelist(id);
}

void
JobWidget::emptyWhitelist()
{
    m_filter->emptyWhitelist();
}

void
JobWidget::addBlacklist(const Tsq::Uuid &id)
{
    m_filter->addBlacklist(id);
}

void
JobWidget::resetFilter()
{
    m_filter->resetFilter();
}

void
JobWidget::removeClosedTerminals()
{
    m_filter->model()->removeClosedTerminals();
}

void
JobWidget::contextMenu()
{
    QPoint mid(width() / 2, height() / 4);
    m_view->contextMenu(mapToGlobal(mid));
}
