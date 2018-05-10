// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/iconbutton.h"
#include "app/selhelper.h"
#include "startupdialog.h"
#include "settings.h"
#include "profile.h"

#include <QStandardItemModel>
#include <QHBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QEvent>

#define TR_BUTTON0 TL("input-button", "Add Server Default")
#define TR_BUTTON1 TL("input-button", "Add Global Default")
#define TR_BUTTON2 TL("input-button", "Add Item")
#define TR_BUTTON3 TL("input-button", "Remove Item")
#define TR_BUTTON4 TL("input-button", "Move Up")
#define TR_BUTTON5 TL("input-button", "Move Down")
#define TR_BUTTON6 TL("input-button", "Clear")
#define TR_TEXT1 TL("window-text", "Profiles")
#define TR_TEXT2 TL("window-text", "Startup Terminals")
#define TR_TITLE1 TL("window-title", "Manage Startup Terminals")

StartupDialog::StartupDialog(const SettingDef *def, SettingsBase *settings, QWidget *parent) :
    QDialog(parent),
    m_def(def),
    m_settings(settings)
{
    setWindowTitle(TR_TITLE1);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    m_profileModel = new QStandardItemModel(this);
    m_profileModel->setColumnCount(1);
    m_profileModel->setHorizontalHeaderLabels(QStringList(TR_TEXT1));
    m_selectModel = new QStandardItemModel(this);
    m_selectModel->setColumnCount(1);
    m_selectModel->setHorizontalHeaderLabels(QStringList(TR_TEXT2));

    m_profileView = new QTableView;
    QItemSelectionModel *m = m_profileView->selectionModel();
    m_profileView->setModel(m_profileModel);
    delete m;
    m_profileView->setShowGrid(false);
    m_profileView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_profileView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_profileView->verticalHeader()->setVisible(false);
    m_profileView->horizontalHeader()->setStretchLastSection(true);

    m_selectView = new QTableView;
    m = m_selectView->selectionModel();
    m_selectView->setModel(m_selectModel);
    delete m;
    m_selectView->setShowGrid(false);
    m_selectView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_selectView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_selectView->verticalHeader()->setVisible(false);
    m_selectView->horizontalHeader()->setStretchLastSection(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Reset,
        Qt::Vertical);

    auto *serverButton = new QPushButton(TR_BUTTON0);
    auto *globalButton = new QPushButton(TR_BUTTON1);
    m_addButton = new IconButton(ICON_NEW_ITEM, TR_BUTTON2);
    m_removeButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON3);
    m_upButton = new IconButton(ICON_MOVE_UP, TR_BUTTON4);
    m_downButton = new IconButton(ICON_MOVE_DOWN, TR_BUTTON5);
    m_clearButton = new IconButton(ICON_CLEAN, TR_BUTTON6);
    QPushButton *resetButton = buttonBox->button(QDialogButtonBox::Reset);

    buttonBox->addButton(serverButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(globalButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_addButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_removeButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_upButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_downButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_clearButton, QDialogButtonBox::ActionRole);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_profileView);
    layout->addWidget(buttonBox);
    layout->addWidget(m_selectView);
    setLayout(layout);

    connect(g_settings, &TermSettings::profilesChanged,
            this, &StartupDialog::bringUp);

    connect(m_profileModel, SIGNAL(modelReset()), SLOT(handleProfileSelect()));
    connect(m_profileView->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleProfileSelect()));

    connect(m_selectModel, SIGNAL(modelReset()), SLOT(handleSelectSelect()));
    connect(m_selectView->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelectSelect()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(serverButton, SIGNAL(clicked()), SLOT(handleAddServer()));
    connect(globalButton, SIGNAL(clicked()), SLOT(handleAddGlobal()));
    connect(m_addButton, SIGNAL(clicked()), SLOT(handleAdd()));
    connect(m_removeButton, SIGNAL(clicked()), SLOT(handleRemove()));
    connect(m_upButton, SIGNAL(clicked()), SLOT(handleMoveUp()));
    connect(m_downButton, SIGNAL(clicked()), SLOT(handleMoveDown()));
    connect(m_clearButton, SIGNAL(clicked()), SLOT(handleClear()));
    connect(resetButton, SIGNAL(clicked()), SLOT(handleReset()));
}

bool
StartupDialog::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        m_profileView->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QDialog::event(event);
}

void
StartupDialog::handleReset()
{
    QHash<QString,QIcon> map;

    m_profileModel->setRowCount(m_profiles.size());
    int row = 0;
    for (auto &i: qAsConst(m_profiles)) {
        m_profileModel->setItem(row, new QStandardItem(i.second, i.first));
        map.insert(i.first,i.second);
        ++row;
    }

    map[g_str_CURRENT_PROFILE] = map[g_str_SERVER_PROFILE] = QIcon();
    QStringList tmp = m_settings->property(m_def->property).toStringList();
    for (QString &k: tmp)
        if (!map.contains(k))
            k = g_str_SERVER_PROFILE;

    m_select.clear();
    m_selectModel->setRowCount(tmp.size());
    row = 0;
    for (auto &i: qAsConst(tmp)) {
        m_selectModel->setItem(row, new QStandardItem(map[i], i));
        m_select.append(std::make_pair(i, map[i]));
        ++row;
    }
}

void
StartupDialog::bringUp()
{
    m_profiles = g_settings->allProfiles();
    handleReset();
}

void
StartupDialog::handleProfileSelect()
{
    QModelIndexList indexes = m_profileView->selectionModel()->selectedIndexes();
    bool enabled = !indexes.isEmpty();

    m_addButton->setEnabled(enabled);

    m_profileRows.clear();
    for (auto &i: qAsConst(indexes)) {
        m_profileRows.insert(i.row(), 1);
    }
}

void
StartupDialog::handleSelectSelect()
{
    QModelIndexList indexes = m_selectView->selectionModel()->selectedIndexes();
    bool enabled = !indexes.isEmpty();

    m_removeButton->setEnabled(enabled);
    m_upButton->setEnabled(enabled);
    m_downButton->setEnabled(enabled);

    m_selectRows.clear();
    for (auto &i: qAsConst(indexes)) {
        m_selectRows.insert(i.row(), 1);
    }
}

void
StartupDialog::addDefault(const QString &str)
{
    int row = m_select.size();
    m_selectModel->insertRows(row, 1);
    m_selectModel->setItem(row, new QStandardItem(str));
    m_select.append(std::make_pair(str, QIcon()));
    m_selectView->selectRow(row);
    m_selectView->scrollToBottom();
}

void
StartupDialog::handleAddServer()
{
    addDefault(g_str_SERVER_PROFILE);
}

void
StartupDialog::handleAddGlobal()
{
    addDefault(g_str_CURRENT_PROFILE);
}

void
StartupDialog::handleAdd()
{
    QItemSelection rowSel;

    for (auto i = m_profileRows.cbegin(), j = m_profileRows.cend(); i != j; ++i)
    {
        const auto &elt = m_profiles[i.key()];
        int row = m_select.size();
        m_selectModel->insertRows(row, 1);
        m_selectModel->setItem(row, new QStandardItem(elt.second, elt.first));
        m_select.append(elt);
        QModelIndex newIndex = m_selectModel->index(row, 0);
        rowSel.select(newIndex, newIndex);
    }

    doSelectItems(m_selectView, rowSel, true);
    m_selectView->scrollToBottom();
}

void
StartupDialog::handleRemove()
{
    const QMap<int,int> copy = m_selectRows;
    for (auto i = copy.cbegin(), j = copy.cend(); i != j; )
    {
        --j;
        m_select.removeAt(j.key());
        m_selectModel->removeRow(j.key());
    }

    m_profileView->setFocus(Qt::OtherFocusReason);
}

void
StartupDialog::handleClear()
{
    m_select.clear();
    m_selectModel->removeRows(0, m_selectModel->rowCount());
    m_profileView->setFocus(Qt::OtherFocusReason);
}

void
StartupDialog::handleMoveUp()
{
    if (m_selectRows.isEmpty() || m_selectRows.contains(0))
        return;

    const QMap<int,int> copy = m_selectRows;
    QItemSelection rowSel;

    for (auto i = copy.cbegin(), j = copy.cend(); i != j; ++i) {
        int row = i.key();
        auto d = m_select[row - 1];
        m_select[row - 1] = m_select[row];
        m_select[row] = d;
        const auto &e = m_select[row - 1];
        m_selectModel->setItem(row - 1, new QStandardItem(e.second, e.first));
        const auto &f = m_select[row];
        m_selectModel->setItem(row, new QStandardItem(f.second, f.first));
    }
    for (auto i = copy.cbegin(), j = copy.cend(); i != j; ++i) {
        QModelIndex newIndex = m_selectModel->index(i.key() - 1, 0);
        rowSel.select(newIndex, newIndex);
    }

    doSelectItems(m_selectView, rowSel, true);
    m_selectView->scrollTo(rowSel.indexes().front());
}

void
StartupDialog::handleMoveDown()
{
    if (m_selectRows.isEmpty() || m_selectRows.contains(m_select.size() - 1))
        return;

    const QMap<int,int> copy = m_selectRows;
    QItemSelection rowSel;

    for (auto i = copy.cbegin(), j = copy.cend(); i != j; ) {
        --j;
        int row = j.key();
        auto d = m_select[row + 1];
        m_select[row + 1] = m_select[row];
        m_select[row] = d;
        const auto &e = m_select[row + 1];
        m_selectModel->setItem(row + 1, new QStandardItem(e.second, e.first));
        const auto &f = m_select[row];
        m_selectModel->setItem(row, new QStandardItem(f.second, f.first));
    }
    for (auto i = copy.cbegin(), j = copy.cend(); i != j; ) {
        --j;
        QModelIndex newIndex = m_selectModel->index(j.key() + 1, 0);
        rowSel.select(newIndex, newIndex);
    }

    doSelectItems(m_selectView, rowSel, true);
    m_selectView->scrollTo(rowSel.indexes().front());
}

void
StartupDialog::handleAccept()
{
    QStringList tmp;
    for (auto &i: qAsConst(m_select))
        tmp.append(i.first);

    m_settings->setProperty(m_def->property, tmp);
    accept();
}
