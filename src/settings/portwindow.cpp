// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "base/listener.h"
#include "base/manager.h"
#include "base/server.h"
#include "base/portouttask.h"
#include "base/portintask.h"
#include "base/taskstatus.h"
#include "portwindow.h"
#include "portmodel.h"
#include "porteditor.h"
#include "settings.h"
#include "settingswindow.h"
#include "state.h"

#include <QHBoxLayout>
#include <QKeyEvent>

#define TR_BUTTON1 TL("input-button", "New Task") + A("...")
#define TR_BUTTON2 TL("input-button", "Edit Server") + A("...")
#define TR_BUTTON3 TL("input-button", "Start Task")
#define TR_BUTTON4 TL("input-button", "Cancel Task")
#define TR_BUTTON5 TL("input-button", "End Connection")
#define TR_ERROR1 TL("error", "This rule is already present in the list")
#define TR_TITLE1 TL("window-title", "Manage Port Forwarding")

PortsWindow *g_portwin;

PortsWindow::PortsWindow()
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_view = new PortView;

    m_newButton = new IconButton(ICON_NEW_ITEM, TR_BUTTON1);
    m_editButton = new IconButton(ICON_EDIT_ITEM, TR_BUTTON2);
    m_startButton = new IconButton(ICON_CONNECTION_LAUNCH, TR_BUTTON3);
    m_cancelButton = new IconButton(ICON_CANCEL_TASK, TR_BUTTON4);
    m_killButton = new IconButton(ICON_CONNECTION_CLOSE, TR_BUTTON5);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Vertical);
    buttonBox->addButton(m_newButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_editButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_startButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_cancelButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_killButton, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("port-forwarding");

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(m_view);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));
    connect(m_view, SIGNAL(portChanged()), SLOT(handleSelection()));
    connect(m_view, SIGNAL(launched()), SLOT(handleLaunch()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(close()));
    connect(m_newButton, SIGNAL(clicked()), SLOT(handleNewPort()));
    connect(m_editButton, SIGNAL(clicked()), SLOT(handleEditServer()));
    connect(m_startButton, SIGNAL(clicked()), SLOT(handleStartPort()));
    connect(m_cancelButton, SIGNAL(clicked()), SLOT(handleCancelPort()));
    connect(m_killButton, SIGNAL(clicked()), SLOT(handleKillConn()));

    handleSelection();
}

bool
PortsWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        g_state->store(PortsGeometryKey, saveGeometry());
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
PortsWindow::bringUp()
{
    restoreGeometry(g_state->fetch(PortsGeometryKey));

    m_view->expandAll();

    show();
    raise();
    activateWindow();
}

void
PortsWindow::handleSelection()
{
    QModelIndex i = m_view->selectedIndex();

    if (!i.isValid()) {
        m_newButton->setEnabled(false);
        m_editButton->setEnabled(false);
        m_startButton->setEnabled(false);
        m_cancelButton->setEnabled(false);
        m_killButton->setEnabled(false);
    } else {
        m_newButton->setEnabled(true);
        m_editButton->setEnabled(true);
        m_startButton->setEnabled(i.data(PORT_ROLE_STARTABLE).toBool());
        m_cancelButton->setEnabled(i.data(PORT_ROLE_CANCELABLE).toBool());
        m_killButton->setEnabled(i.data(PORT_ROLE_KILLABLE).toBool());
    }
}

void
PortsWindow::handleNewPort()
{
    ServerInstance *server = PORT_SERVERP(m_view->selectedIndex());
    TermManager *manager = g_listener->activeManager();
    if (!server || !manager)
        return;

    if (!m_dialog)
        m_dialog = new PortEditor(this, true);
    m_dialog->setServer(server);
    if (m_dialog->exec() != QDialog::Accepted)
        return;

    PortFwdRule rule = m_dialog->buildRule();

    if (m_view->hasPort(server, &rule))
        errBox(TR_ERROR1, this)->show();
    else if (rule.islocal)
        (new PortOutTask(server, rule))->start(manager);
    else
        (new PortInTask(server, rule))->start(manager);
}

void
PortsWindow::handleEditServer()
{
    auto *server = PORT_SERVERP(m_view->selectedIndex());
    if (server)
        g_settings->serverWindow(server->serverInfo())->bringUp();
}

void
PortsWindow::handleStartPort()
{
    QModelIndex index = m_view->selectedIndex();
    auto *server = PORT_SERVERP(index);
    const auto *rule = PORT_RULEP(index);
    auto *manager = g_listener->activeManager();

    if (rule && manager) {
        if (rule->islocal)
            (new PortOutTask(server, *rule))->start(manager);
        else
            (new PortInTask(server, *rule))->start(manager);
    }
}

void
PortsWindow::handleInspectPort()
{
    auto *task = PORT_TASKP(m_view->selectedIndex());
    auto *manager = g_listener->activeManager();

    if (task && manager)
        (new TaskStatus(task, manager->parent()))->show();
}

void
PortsWindow::handleCancelPort()
{
    auto *task = PORT_TASKP(m_view->selectedIndex());
    if (task)
        task->cancel();
}

void
PortsWindow::handleKillConn()
{
    QModelIndex index = m_view->selectedIndex();
    auto *task = PORT_TASKP(index);
    if (task)
        task->killConnection(index.data(PORT_ROLE_CONN).toUInt());
}

void
PortsWindow::handleLaunch()
{
    QModelIndex index = m_view->selectedIndex();

    switch (index.data(PORT_ROLE_TYPE).toInt()) {
    case PortModel::LevelServer:
        handleEditServer();
        break;
    case PortModel::LevelPort:
        if (index.data(PORT_ROLE_STARTABLE).toBool())
            handleStartPort();
        else if (index.data(PORT_ROLE_CANCELABLE).toBool())
            handleInspectPort();
        break;
    }
}
