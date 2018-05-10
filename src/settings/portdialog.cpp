// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "portdialog.h"
#include "porteditmodel.h"
#include "porteditor.h"

#include <QHBoxLayout>
#include <QEvent>

#define TR_BUTTON1 TL("input-button", "Add Item")
#define TR_BUTTON2 TL("input-button", "Remove Item")
#define TR_BUTTON3 TL("input-button", "Edit Item")
#define TR_ERROR1 TL("error", "This rule is already present in the list")
#define TR_TITLE1 TL("window-title", "Edit Port Forwarding")

PortsDialog::PortsDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(TR_TITLE1);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    m_model = new PortEditModel(this);
    m_view = new PortEditView(m_model);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel,
        Qt::Vertical);

    m_addButton = new IconButton(ICON_NEW_ITEM, TR_BUTTON1);
    m_removeButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON2);
    m_editButton = new IconButton(ICON_EDIT_ITEM, TR_BUTTON3);

    buttonBox->addButton(m_addButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_removeButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_editButton, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("port-forwarding");

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_view);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_model, SIGNAL(modelReset()), SLOT(handleSelect()));
    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelect()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(m_addButton, SIGNAL(clicked()), SLOT(handleAdd()));
    connect(m_removeButton, SIGNAL(clicked()), SLOT(handleRemove()));
    connect(m_editButton, SIGNAL(clicked()), SLOT(handleEdit()));
}

bool
PortsDialog::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        m_view->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QDialog::event(event);
}

void
PortsDialog::bringUp()
{
    m_view->resizeColumnsToContents();
    handleSelect();
}

void
PortsDialog::handleSelect()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedIndexes();
    bool enabled = !indexes.isEmpty();

    m_removeButton->setEnabled(enabled);
    m_editButton->setEnabled(enabled);
}

void
PortsDialog::handleAdd()
{
    if (!m_dialog)
        m_dialog = new PortEditor(this, false);
    m_dialog->setAdding();
    if (m_dialog->exec() != QDialog::Accepted)
        return;

    PortFwdRule rule = m_dialog->buildRule();

    if (m_model->containsRule(rule)) {
        errBox(TR_ERROR1, this)->show();
    } else {
        m_view->selectRow(m_model->addRule(rule));
    }
}

void
PortsDialog::handleEdit()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return;

    int row = indexes.at(0).row();

    if (!m_dialog)
        m_dialog = new PortEditor(this, false);
    m_dialog->setRule(m_model->rule(row));
    if (m_dialog->exec() != QDialog::Accepted)
        return;

    PortFwdRule rule = m_dialog->buildRule();

    if (m_model->containsRule(rule)) {
        errBox(TR_ERROR1, this)->show();
    } else {
        m_model->setRule(row, rule);
    }
}

void
PortsDialog::handleRemove()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty())
        m_model->removeRule(indexes.at(0).row());
}
