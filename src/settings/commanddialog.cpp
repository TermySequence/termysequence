// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "commanddialog.h"
#include "base.h"

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QEvent>

#define TR_BUTTON1 TL("input-button", "Insert Item")
#define TR_BUTTON2 TL("input-button", "Prepend Item")
#define TR_BUTTON3 TL("input-button", "Append Item")
#define TR_BUTTON4 TL("input-button", "Delete Item")
#define TR_BUTTON5 TL("input-button", "Move Up")
#define TR_BUTTON6 TL("input-button", "Move Down")
#define TR_FIELD1 TL("input-field", "Executable path") + ':'
#define TR_FIELD2 TL("input-field", "Argument vector") + ':'
#define TR_TITLE1 TL("window-title", "Edit Command Line")

#define NEWITEM(...) \
    QListWidgetItem *item = new QListWidgetItem( __VA_ARGS__); \
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable)

CommandDialog::CommandDialog(const SettingDef *def, SettingsBase *settings, QWidget *parent) :
    QDialog(parent),
    m_def(def),
    m_settings(settings)
{
    setWindowTitle(TR_TITLE1);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    m_prog = new QLineEdit;
    m_view = new QListWidget;
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::ContiguousSelection);
    m_view->setEditTriggers(QAbstractItemView::AllEditTriggers);

    m_insertButton = new IconButton(ICON_INSERT_ITEM, TR_BUTTON1);
    m_prependButton = new IconButton(ICON_PREPEND_ITEM, TR_BUTTON2);
    m_appendButton = new IconButton(ICON_APPEND_ITEM, TR_BUTTON3);
    m_deleteButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON4);
    m_upButton = new IconButton(ICON_MOVE_UP, TR_BUTTON5);
    m_downButton = new IconButton(ICON_MOVE_DOWN, TR_BUTTON6);

    QDialogButtonBox *listBox = new QDialogButtonBox(0, Qt::Vertical);
    listBox->addButton(m_prependButton, QDialogButtonBox::ActionRole);
    listBox->addButton(m_insertButton, QDialogButtonBox::ActionRole);
    listBox->addButton(m_appendButton, QDialogButtonBox::ActionRole);
    listBox->addButton(m_deleteButton, QDialogButtonBox::ActionRole);
    listBox->addButton(m_upButton, QDialogButtonBox::ActionRole);
    listBox->addButton(m_downButton, QDialogButtonBox::ActionRole);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Reset);

    auto *shButton = new QPushButton(A("/bin/sh"));
    buttonBox->addButton(shButton, QDialogButtonBox::ActionRole);
    m_resetButton = buttonBox->button(QDialogButtonBox::Reset);

    QHBoxLayout *listLayout = new QHBoxLayout;
    listLayout->addWidget(m_view, 1);
    listLayout->addWidget(listBox);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_prog);
    layout->addWidget(new QLabel(TR_FIELD2));
    layout->addLayout(listLayout, 1);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_view->model(), &QAbstractItemModel::dataChanged,
            this, &CommandDialog::handleChange);
    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));

    connect(m_prog, SIGNAL(textEdited(const QString&)), SLOT(handleTextEdited(const QString&)));
    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(m_insertButton, SIGNAL(clicked()), SLOT(handleInsertArg()));
    connect(m_prependButton, SIGNAL(clicked()), SLOT(handlePrependArg()));
    connect(m_appendButton, SIGNAL(clicked()), SLOT(handleAppendArg()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDeleteArg()));
    connect(m_upButton, SIGNAL(clicked()), SLOT(handleUp()));
    connect(m_downButton, SIGNAL(clicked()), SLOT(handleDown()));
    connect(m_resetButton, SIGNAL(clicked()), SLOT(handleReset()));
    connect(shButton, SIGNAL(clicked()), SLOT(handleSh()));

    handleSelection();
}

bool
CommandDialog::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        m_prog->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QDialog::event(event);
}

void
CommandDialog::handleSelection()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    int row = !indexes.isEmpty() ? indexes.at(0).row() : -1;

    m_deleteButton->setEnabled(row > 0);
    m_upButton->setEnabled(row > 1);
    m_downButton->setEnabled(row > 0);
}

void
CommandDialog::handleChange()
{
    m_resetButton->setEnabled(true);
}

void
CommandDialog::handleInsertArg()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (!indexes.isEmpty()) {
        int row = indexes.at(0).row();
        if (row == 0)
            row = 1;
        NEWITEM(A("arg"));
        m_view->insertItem(row, item);
        m_view->clearSelection();
        m_view->setCurrentItem(item);
        m_view->scrollToItem(item);
        handleChange();
    } else {
        handlePrependArg();
    }
}

void
CommandDialog::handlePrependArg()
{
    NEWITEM(A("arg"));
    m_view->insertItem(1, item);
    m_view->clearSelection();
    m_view->setCurrentItem(item);
    m_view->scrollToItem(item);
    handleChange();
}

void
CommandDialog::handleAppendArg()
{
    NEWITEM(A("arg"), m_view);
    m_view->clearSelection();
    m_view->setCurrentItem(item);
    m_view->scrollToItem(item);
    handleChange();
}

void
CommandDialog::handleDeleteArg()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (indexes.isEmpty())
        return;

    int row = indexes.at(0).row();
    if (row > 0) {
        delete m_view->item(row);
        auto *item = m_view->item(row - 1);
        m_view->clearSelection();
        m_view->setCurrentItem(item);
        m_view->scrollToItem(item);
        handleChange();
    }
}

void
CommandDialog::handleUp()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (indexes.isEmpty())
        return;

    int row = indexes.at(0).row();
    if (row > 1) {
        auto *item = m_view->takeItem(row);
        m_view->insertItem(row - 1, item);
        m_view->clearSelection();
        m_view->setCurrentItem(item);
        m_view->scrollToItem(item);
    }
}

void
CommandDialog::handleDown()
{
    QModelIndexList indexes = m_view->selectionModel()->selectedRows(0);
    if (indexes.isEmpty())
        return;

    int row = indexes.at(0).row();
    if (row > 0 && row < m_view->count() - 1) {
        auto *item = m_view->takeItem(row);
        m_view->insertItem(row + 1, item);
        m_view->clearSelection();
        m_view->setCurrentItem(item);
        m_view->scrollToItem(item);
    }
}

void
CommandDialog::handleReset()
{
    setContent(m_saved);
}

void
CommandDialog::handleAccept()
{
    QStringList list(m_prog->text());

    for (int i = 0, n = m_view->count(); i < n; ++i)
        list.append(m_view->item(i)->text());

    m_settings->setProperty(m_def->property, list);
    accept();
}

void
CommandDialog::handleTextEdited(const QString &text)
{
    m_view->item(0)->setText(text);
}

void
CommandDialog::setContent(QStringList list)
{
    m_saved = list;
    m_resetButton->setEnabled(false);

    if (list.isEmpty())
        list.append(g_mtstr);
    if (list.size() == 1)
        list.append(list.front());

    m_view->clear();
    for (int i = 1, n = list.size(); i < n; ++i) {
        NEWITEM(list[i], m_view);
    }

    m_prog->setText(list.front());
}

void
CommandDialog::handleSh()
{
    m_prog->setText(A("/bin/sh"));
    m_view->clear();

    { NEWITEM(A("/bin/sh"), m_view); }
    { NEWITEM(A("-c"), m_view); }
    NEWITEM(A("command..."), m_view);
    m_view->clearSelection();
    m_view->setCurrentItem(item);
    handleChange();
}
