// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/enums.h"
#include "app/iconbutton.h"
#include "app/selhelper.h"
#include "batchdialog.h"
#include "batchmodel.h"
#include "connect.h"

#include <QMap>
#include <QLineEdit>
#include <QHBoxLayout>

#define TR_BUTTON1 TL("input-button", "Insert Rule")
#define TR_BUTTON2 TL("input-button", "Prepend Rule")
#define TR_BUTTON3 TL("input-button", "Append Rule")
#define TR_BUTTON4 TL("input-button", "Delete Rule")
#define TR_BUTTON5 TL("input-button", "Move to Top")
#define TR_BUTTON6 TL("input-button", "Move Up")
#define TR_BUTTON7 TL("input-button", "Move Down")
#define TR_BUTTON8 TL("input-button", "Move to Bottom")
#define TR_BUTTON9 TL("input-button", "Clone Rule")
#define TR_TITLE1 TL("window-title", "New Batch Connection")
#define TR_TITLE2 TL("window-title", "Edit Batch Connection")

BatchDialog::BatchDialog(QWidget *parent, unsigned options, ConnectSettings *conninfo) :
    ConnectDialog(parent, "connect-batch", options),
    m_editing(conninfo)
{
    setWindowTitle(m_editing ? TR_TITLE2 : TR_TITLE1);

    if (m_editing) {
        (m_info = conninfo)->takeReference();
    } else {
        m_info = new ConnectSettings;
        m_info->activate();
        m_info->setType(Tsqt::ConnectionBatch);
    }
    m_info->activate();
    m_model = new BatchModel(m_info->batch(), this);
    m_view = new BatchView(m_model);

    m_insertButton = new IconButton(ICON_NEW_ITEM, TR_BUTTON1);
    m_prependButton = new IconButton(ICON_PREPEND_ITEM, TR_BUTTON2);
    m_appendButton = new IconButton(ICON_APPEND_ITEM, TR_BUTTON3);
    m_deleteButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON4);
    m_cloneButton = new IconButton(ICON_CLONE_ITEM, TR_BUTTON9);

    m_firstButton = new IconButton(ICON_MOVE_TOP, TR_BUTTON5);
    m_upButton = new IconButton(ICON_MOVE_UP, TR_BUTTON6);
    m_downButton = new IconButton(ICON_MOVE_DOWN, TR_BUTTON7);
    m_lastButton = new IconButton(ICON_MOVE_BOTTOM, TR_BUTTON8);

    auto mask = m_editing ?
        QDialogButtonBox::Apply|QDialogButtonBox::Reset :
        QDialogButtonBox::NoButton;
    QDialogButtonBox *buttonBox = new QDialogButtonBox(mask, Qt::Vertical);

    buttonBox->addButton(m_appendButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_insertButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_prependButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_cloneButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_deleteButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_firstButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_upButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_downButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_lastButton, QDialogButtonBox::ActionRole);

    if (m_editing) {
        m_applyButton = buttonBox->button(QDialogButtonBox::Apply);
        m_resetButton = buttonBox->button(QDialogButtonBox::Reset);
        m_applyButton->setEnabled(false);
        m_resetButton->setEnabled(false);
        connect(m_applyButton, SIGNAL(clicked()), SLOT(handleApply()));
        connect(m_resetButton, SIGNAL(clicked()), SLOT(handleReset()));
    }

    auto *ourLayout = new QHBoxLayout;
    ourLayout->addWidget(m_view, 1);
    ourLayout->addWidget(buttonBox);
    m_mainLayout->insertLayout(0, ourLayout);

    connect(m_model, &BatchModel::dataChanged, this, &BatchDialog::handleChange);
    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));

    connect(m_insertButton, SIGNAL(clicked()), SLOT(handleInsertRule()));
    connect(m_prependButton, SIGNAL(clicked()), SLOT(handlePrependRule()));
    connect(m_appendButton, SIGNAL(clicked()), SLOT(handleAppendRule()));
    connect(m_cloneButton, SIGNAL(clicked()), SLOT(handleCloneRule()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDeleteRule()));
    connect(m_firstButton, SIGNAL(clicked()), SLOT(handleFirst()));
    connect(m_upButton, SIGNAL(clicked()), SLOT(handleUp()));
    connect(m_downButton, SIGNAL(clicked()), SLOT(handleDown()));
    connect(m_lastButton, SIGNAL(clicked()), SLOT(handleLast()));

    handleSelection();
}

void
BatchDialog::handleSelection()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    bool enabled = !indexes.isEmpty();

    m_cloneButton->setEnabled(enabled);
    m_deleteButton->setEnabled(enabled);
    m_firstButton->setEnabled(enabled);
    m_upButton->setEnabled(enabled);
    m_downButton->setEnabled(enabled);
    m_lastButton->setEnabled(enabled);

    m_selLow = m_selHigh = -1;
    for (auto &i: qAsConst(indexes)) {
        int row = i.row();
        if (m_selLow == -1 || m_selLow > row)
            m_selLow = row;
        if (m_selHigh == -1 || m_selHigh < row)
            m_selHigh = row;
    }
}

void
BatchDialog::handleChange()
{
    if (m_editing) {
        m_applyButton->setEnabled(true);
        m_resetButton->setEnabled(true);
    }
}

void
BatchDialog::handleInsertRule()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (!indexes.isEmpty()) {
        int row = indexes.at(0).row();
        m_model->insertRule(row);
        m_view->selectRow(row);
        m_view->scrollTo(m_model->index(row, 0));
        handleChange();
    } else {
        handlePrependRule();
    }
}

void
BatchDialog::handlePrependRule()
{
    m_model->insertRule(0);
    m_view->selectRow(0);
    m_view->scrollToTop();
    handleChange();
}

void
BatchDialog::handleAppendRule()
{
    int row = m_model->rowCount();
    m_model->insertRule(row);
    m_view->selectRow(row);
    m_view->scrollToBottom();
    handleChange();
}

void
BatchDialog::handleCloneRule()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (indexes.isEmpty())
        return;

    int row = indexes.at(0).row();
    m_model->cloneRule(row);
    m_view->selectRow(++row);
    m_view->scrollTo(m_model->index(row, 0));
    handleChange();
}

void
BatchDialog::handleDeleteRule()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (indexes.isEmpty())
        return;

    QMap<int,int> sorted;
    for (const auto &index: qAsConst(indexes))
        sorted[index.row()] = 1;

    int row = sorted.cbegin().key();
    for (auto i = sorted.cbegin(), j = sorted.cend(); i != j; ) {
        --j;
        m_model->removeRule(j.key());
    }

    int nrows = m_model->rowCount();
    if (row == nrows)
        --row;
    if (row >= 0) {
        m_view->selectRow(row);
        m_view->scrollTo(m_model->index(row, 0));
    }
    handleChange();
}

void
BatchDialog::handleFirst()
{
    if (m_selLow <= 0)
        return;

    m_model->moveFirst(m_selLow, m_selHigh);
    QModelIndex selStart = m_model->index(0, 0);
    QModelIndex selEnd = m_model->index(m_selHigh - m_selLow, 0);

    QItemSelection rowSel;
    rowSel.select(selStart, selEnd);
    doSelectItems(m_view, rowSel, true);
    m_view->scrollToTop();
}

void
BatchDialog::handleUp()
{
    if (m_selLow <= 0)
        return;

    m_model->moveUp(m_selLow, m_selHigh);
    QModelIndex selStart = m_model->index(m_selLow - 1, 0);
    QModelIndex selEnd = m_model->index(m_selHigh - 1, 0);

    QItemSelection rowSel;
    rowSel.select(selStart, selEnd);
    doSelectItems(m_view, rowSel, true);
    m_view->scrollTo(selStart);
}

void
BatchDialog::handleDown()
{
    if (m_selHigh == -1 || m_selHigh == m_model->rowCount() - 1)
        return;

    m_model->moveDown(m_selLow, m_selHigh);
    QModelIndex selStart = m_model->index(m_selLow + 1, 0);
    QModelIndex selEnd = m_model->index(m_selHigh + 1, 0);

    QItemSelection rowSel;
    rowSel.select(selStart, selEnd);
    doSelectItems(m_view, rowSel, true);
    m_view->scrollTo(selEnd);
}

void
BatchDialog::handleLast()
{
    int last = m_model->rowCount() - 1;

    if (m_selHigh == -1 || m_selHigh == last)
        return;

    m_model->moveLast(m_selLow, m_selHigh);
    QModelIndex selStart = m_model->index(last - (m_selHigh - m_selLow), 0);
    QModelIndex selEnd = m_model->index(last, 0);

    QItemSelection rowSel;
    rowSel.select(selStart, selEnd);
    doSelectItems(m_view, rowSel, true);
    m_view->scrollToBottom();
}

void
BatchDialog::handleAccept()
{
    m_info->setBatch(m_model->result());

    if (m_editing) {
        m_info->saveSettings();
        accept();
    } else {
        doSave(m_saveName->text());
    }
}

void
BatchDialog::handleApply()
{
    m_info->setBatch(m_model->result());
    m_info->saveSettings();
    m_applyButton->setEnabled(false);
    m_resetButton->setEnabled(false);
}

void
BatchDialog::handleReset()
{
    if (m_applyButton->isEnabled()) {
        m_model->loadRules(m_info->batch());
        m_applyButton->setEnabled(false);
        m_resetButton->setEnabled(false);
    }
    m_view->reset();
}
