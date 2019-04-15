// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "base/listener.h"
#include "base/manager.h"
#include "base/mainwindow.h"
#include "base/server.h"
#include "base/term.h"
#include "termwindow.h"
#include "termmodel.h"
#include "state.h"

#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>

#define STATE_VERSION 1

#define TR_ASK1 TL("question", "Really disconnect server \"%1\"?")
#define TR_BUTTON0 TL("input-button", "Sync Order to All Windows")
#define TR_BUTTON1 TL("input-button", "Switch To")
#define TR_BUTTON2 TL("input-button", "Move to Front")
#define TR_BUTTON3 TL("input-button", "Move Up")
#define TR_BUTTON4 TL("input-button", "Move Down")
#define TR_BUTTON5 TL("input-button", "Move to Back")
#define TR_BUTTON6 TL("input-button", "Hide")
#define TR_BUTTON7 TL("input-button", "Show")
#define TR_BUTTON8 TL("input-button", "New Terminal")
#define TR_BUTTON9 TL("input-button", "Close Terminal")
#define TR_BUTTON10 TL("input-button", "Disconnect")
#define TR_CHECK1 TL("window-text", "Terminal colors")
#define TR_FIELD1 TL("window-text", "Window") + ':'
#define TR_TITLE1 TL("window-title", "Manage Terminals")
#define TR_TITLE2 TL("window-title", "Confirm Disconnect")

TermsWindow *g_termwin;

TermsWindow::TermsWindow(TermManager *manager)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_manager = manager;
    m_model = new TermModel(manager, this);
    m_view = new TermView(m_model);

    m_check = new QCheckBox(TR_CHECK1);
    m_combo = new QComboBox;

    for (auto manager: g_listener->managers())
        handleManagerAdded(manager);

    QVariant val = QVariant::fromValue((QObject*)manager);
    m_combo->setCurrentIndex(m_combo->findData(val));

    auto *syncButton = new QPushButton(TR_BUTTON0);
    m_switchButton = new IconButton(ICON_SWITCH_ITEM, TR_BUTTON1);
    m_frontButton = new IconButton(ICON_MOVE_TOP, TR_BUTTON2);
    m_upButton = new IconButton(ICON_MOVE_UP, TR_BUTTON3);
    m_downButton = new IconButton(ICON_MOVE_DOWN, TR_BUTTON4);
    m_backButton = new IconButton(ICON_MOVE_BOTTOM, TR_BUTTON5);
    m_hideButton = new IconButton(ICON_HIDE_ITEM, TR_BUTTON6);
    m_showButton = new IconButton(ICON_SHOW_ITEM, TR_BUTTON7);
    m_newButton = new IconButton(ICON_NEW_ITEM, TR_BUTTON8);
    m_closeButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON9);
    m_stopButton = new IconButton(ICON_CONNECTION_CLOSE, TR_BUTTON10);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Vertical);
    buttonBox->addButton(m_switchButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_frontButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_upButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_downButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_backButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_hideButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_showButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_newButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_closeButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_stopButton, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("manage-terminals");

    QHBoxLayout *upperLayout = new QHBoxLayout;
    upperLayout->setContentsMargins(g_mtmargins);
    upperLayout->addWidget(new QLabel(TR_FIELD1));
    upperLayout->addWidget(m_combo, 1);
    upperLayout->addWidget(syncButton);
    upperLayout->addWidget(m_check);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(g_mtmargins);
    layout->addWidget(m_view, 1);
    layout->addWidget(buttonBox);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(upperLayout);
    mainLayout->addLayout(layout, 1);
    setLayout(mainLayout);

    connect(g_listener, &TermListener::managerRemoved, this, &TermsWindow::handleManagerRemoved);
    connect(g_listener, &TermListener::managerAdded, this, &TermsWindow::handleManagerAdded);

    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));
    connect(m_view, SIGNAL(launched()), SLOT(handleSwitch()));

    connect(syncButton, SIGNAL(clicked()), SLOT(handleSync()));
    connect(m_check, SIGNAL(toggled(bool)), SLOT(handleCheckBox()));
    connect(m_combo, SIGNAL(currentIndexChanged(int)), SLOT(handleIndexChanged()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(close()));
    connect(m_switchButton, SIGNAL(clicked()), SLOT(handleSwitch()));
    connect(m_frontButton, SIGNAL(clicked()), SLOT(handleMoveFront()));
    connect(m_upButton, SIGNAL(clicked()), SLOT(handleMoveUp()));
    connect(m_downButton, SIGNAL(clicked()), SLOT(handleMoveDown()));
    connect(m_backButton, SIGNAL(clicked()), SLOT(handleMoveBack()));
    connect(m_hideButton, SIGNAL(clicked()), SLOT(handleHide()));
    connect(m_showButton, SIGNAL(clicked()), SLOT(handleShow()));
    connect(m_newButton, SIGNAL(clicked()), SLOT(handleNew()));
    connect(m_closeButton, SIGNAL(clicked()), SLOT(handleClose()));
    connect(m_stopButton, SIGNAL(clicked()), SLOT(handleStop()));

    handleSelection();
}

bool
TermsWindow::event(QEvent *event)
{
    QByteArray setting;

    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        g_state->store(TermsGeometryKey, saveGeometry());
        setting.push_back(m_check->isChecked());
        g_state->storeVersioned(TermsSettingsKey, STATE_VERSION, setting);
        break;
    case QEvent::WindowActivate:
        m_view->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QWidget::event(event);
}

void
TermsWindow::bringUp()
{
    restoreGeometry(g_state->fetch(TermsGeometryKey));
    auto settings = g_state->fetchVersioned(TermsSettingsKey, STATE_VERSION);
    m_check->setChecked(!settings.isEmpty() && settings[0]);

    m_view->expandAll();

    show();
    raise();
    activateWindow();
}

void
TermsWindow::handleManagerAdded(TermManager *manager)
{
    QVariant val = QVariant::fromValue((QObject*)manager);
    m_combo->addItem(manager->title(), val);
    connect(manager, &TermManager::titleChanged, this, &TermsWindow::handleTitle);
}

void
TermsWindow::handleManagerRemoved(TermManager *manager)
{
    QVariant val = QVariant::fromValue((QObject*)manager);
    m_combo->removeItem(m_combo->findData(val));
}

void
TermsWindow::handleTitle(const QString &title)
{
    QVariant val = QVariant::fromValue(sender());
    m_combo->setItemText(m_combo->findData(val), title);
}

void
TermsWindow::handleIndexChanged()
{
    QVariant val = m_combo->currentData();
    m_manager = static_cast<TermManager*>(val.value<QObject*>());
    m_model->setManager(m_manager);
}

void
TermsWindow::handleSync()
{
    for (auto manager: g_listener->managers())
        if (manager != m_manager)
            manager->syncTo(m_manager);
}

void
TermsWindow::handleCheckBox()
{
    m_model->setColors(m_check->isChecked());
}

void
TermsWindow::handleSelection()
{
    QModelIndex index = m_view->selectedIndex();
    auto *server = TERM_SERVERP(index);
    auto *term = TERM_TERMP(index);

    m_switchButton->setEnabled(server);
    m_frontButton->setEnabled(server);
    m_upButton->setEnabled(server);
    m_downButton->setEnabled(server);
    m_backButton->setEnabled(server);
    m_hideButton->setEnabled(server);
    m_showButton->setEnabled(server);
    m_closeButton->setEnabled(term);
    m_stopButton->setEnabled(server);
}

void
TermsWindow::handleSwitch()
{
    QModelIndex index = m_view->selectedIndex();
    switch (index.data(TERM_ROLE_TYPE).toInt()) {
    case TermModel::LevelServer:
        m_manager->actionSwitchServer(TERM_SERVERP(index)->idStr(), g_mtstr);
        break;
    case TermModel::LevelTerm:
        m_manager->actionSwitchTerminal(TERM_TERMP(index)->idStr(), g_mtstr);
        break;
    }

    m_manager->parent()->raise();
    m_manager->parent()->activateWindow();
}

void
TermsWindow::handleMoveFront()
{
    QModelIndex index = m_view->selectedIndex();
    switch (index.data(TERM_ROLE_TYPE).toInt()) {
    case TermModel::LevelServer:
        m_manager->reorderServerFirst(TERM_SERVERP(index));
        break;
    case TermModel::LevelTerm:
        m_manager->reorderTermFirst(TERM_TERMP(index));
        break;
    }
}

void
TermsWindow::handleMoveUp()
{
    QModelIndex index = m_view->selectedIndex();
    switch (index.data(TERM_ROLE_TYPE).toInt()) {
    case TermModel::LevelServer:
        m_manager->reorderServerForward(TERM_SERVERP(index));
        break;
    case TermModel::LevelTerm:
        m_manager->reorderTermForward(TERM_TERMP(index));
        break;
    }
}

void
TermsWindow::handleMoveDown()
{
    QModelIndex index = m_view->selectedIndex();
    switch (index.data(TERM_ROLE_TYPE).toInt()) {
    case TermModel::LevelServer:
        m_manager->reorderServerBackward(TERM_SERVERP(index));
        break;
    case TermModel::LevelTerm:
        m_manager->reorderTermBackward(TERM_TERMP(index));
        break;
    }
}

void
TermsWindow::handleMoveBack()
{
    QModelIndex index = m_view->selectedIndex();
    switch (index.data(TERM_ROLE_TYPE).toInt()) {
    case TermModel::LevelServer:
        m_manager->reorderServerLast(TERM_SERVERP(index));
        break;
    case TermModel::LevelTerm:
        m_manager->reorderTermLast(TERM_TERMP(index));
        break;
    }
}

void
TermsWindow::handleHide()
{
    QModelIndex index = m_view->selectedIndex();
    switch (index.data(TERM_ROLE_TYPE).toInt()) {
    case TermModel::LevelServer:
        m_manager->setHidden(TERM_SERVERP(index), true);
        break;
    case TermModel::LevelTerm:
        m_manager->setHidden(TERM_TERMP(index), true);
        break;
    }
}

void
TermsWindow::handleShow()
{
    QModelIndex index = m_view->selectedIndex();
    switch (index.data(TERM_ROLE_TYPE).toInt()) {
    case TermModel::LevelServer:
        m_manager->setHidden(TERM_SERVERP(index), false);
        break;
    case TermModel::LevelTerm:
        m_manager->setHidden(TERM_TERMP(index), false);
        break;
    }
}

void
TermsWindow::handleNew()
{
    auto *server = TERM_SERVERP(m_view->selectedIndex());
    m_manager->actionNewTerminal(g_mtstr, server ? server->idStr() : g_mtstr);
}

void
TermsWindow::handleClose()
{
    auto *term = TERM_TERMP(m_view->selectedIndex());
    if (term)
        term->close(m_manager);
}

void
TermsWindow::handleStop()
{
    auto *server = TERM_SERVERP(m_view->selectedIndex());
    if (!server)
        return;

    QString name = server->shortname();

    if (QMessageBox::Yes == askBox(TR_TITLE2, TR_ASK1.arg(name), this)->exec())
        server->unconnect();
}
