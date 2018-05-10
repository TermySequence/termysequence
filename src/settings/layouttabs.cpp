// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "layouttabs.h"
#include "layoutmodel.h"
#include "fillmodel.h"

#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Move Left")
#define TR_BUTTON2 TL("input-button", "Move Right")
#define TR_BUTTON3 TL("input-button", "Add Item")
#define TR_BUTTON4 TL("input-button", "Edit Item")
#define TR_BUTTON5 TL("input-button", "Delete Item")
#define TR_TAB1 TL("tab-title", "Widget Layout")
#define TR_TAB2 TL("tab-title", "Column Fills")

LayoutTabs::LayoutTabs(TermLayout &layout, const Termcolors &tcpal, const QFont &font) :
    m_layout(layout)
{
    m_lview = new LayoutView(m_layout);
    m_fview = new FillView(m_layout, tcpal, font);

    // Layout tab
    QDialogButtonBox *buttonBox = new QDialogButtonBox(0, Qt::Vertical);
    m_upButton = new IconButton(ICON_MOVE_UP, TR_BUTTON1);
    m_downButton = new IconButton(ICON_MOVE_DOWN, TR_BUTTON2);
    buttonBox->addButton(m_upButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_downButton, QDialogButtonBox::ActionRole);

    QHBoxLayout *tabLayout = new QHBoxLayout;
    tabLayout->addWidget(m_lview);
    tabLayout->addWidget(buttonBox);

    auto *tab = new QWidget;
    tab->setLayout(tabLayout);
    addTab(tab, TR_TAB1);

    // Fills tab
    buttonBox = new QDialogButtonBox(0, Qt::Vertical);
    auto *addButton = new IconButton(ICON_NEW_ITEM, TR_BUTTON3);
    m_editButton = new IconButton(ICON_EDIT_ITEM, TR_BUTTON4);
    m_deleteButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON5);
    buttonBox->addButton(addButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_editButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_deleteButton, QDialogButtonBox::ActionRole);

    tabLayout = new QHBoxLayout;
    tabLayout->addWidget(m_fview);
    tabLayout->addWidget(buttonBox);

    tab = new QWidget;
    tab->setLayout(tabLayout);
    addTab(tab, TR_TAB2);

    connect(m_lview, SIGNAL(modified()), SIGNAL(modified()));
    connect(m_fview, SIGNAL(modified()), SIGNAL(modified()));
    connect(m_lview->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelect()));
    connect(m_fview->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelect()));
    connect(m_fview, SIGNAL(modelReset()), SLOT(handleSelect()));

    connect(m_upButton, SIGNAL(clicked()), SLOT(handleMoveUp()));
    connect(m_downButton, SIGNAL(clicked()), SLOT(handleMoveDown()));
    connect(addButton, &QPushButton::clicked, m_fview, &FillView::addRule);
    connect(m_editButton, &QPushButton::clicked, m_fview, &FillView::editRule);
    connect(m_deleteButton, &QPushButton::clicked, m_fview, &FillView::deleteRule);

    handleSelect();
}

void
LayoutTabs::setExclusiveTab(int tab)
{
    setCurrentIndex(tab);
    setTabEnabled(!tab, false);
    tabBar()->setVisible(false);
}

void
LayoutTabs::reloadData()
{
    m_lview->reloadData();
    m_fview->reloadData();
}

void
LayoutTabs::handleSelect()
{
    QModelIndexList indexes = m_lview->selectionModel()->selectedIndexes();
    bool enabled = !indexes.isEmpty();

    m_upButton->setEnabled(enabled);
    m_downButton->setEnabled(enabled);

    indexes = m_fview->selectionModel()->selectedIndexes();
    enabled = !indexes.isEmpty();

    m_editButton->setEnabled(enabled);
    m_deleteButton->setEnabled(enabled);
}

void
LayoutTabs::handleMoveUp()
{
    QModelIndexList indexes = m_lview->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty())
        m_lview->moveUp(indexes.at(0).row());
}

void
LayoutTabs::handleMoveDown()
{
    QModelIndexList indexes = m_lview->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty())
        m_lview->moveDown(indexes.at(0).row());
}
