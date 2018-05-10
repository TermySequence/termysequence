// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/exception.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "serverwindow.h"
#include "servermodel.h"
#include "servinfo.h"
#include "settings.h"
#include "settingswindow.h"
#include "state.h"

#include <QHBoxLayout>
#include <QKeyEvent>

#define TR_ASK1 TL("question", "Really delete server \"%1\"?")
#define TR_BUTTON1 TL("input-button", "Edit Item") + A("...")
#define TR_BUTTON2 TL("input-button", "Delete Item")
#define TR_BUTTON3 TL("input-button", "Reload Files")
#define TR_TITLE1 TL("window-title", "Manage Servers")
#define TR_TITLE2 TL("window-title", "Confirm Delete")

ServersWindow *g_serverwin;

ServersWindow::ServersWindow()
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_view = new ServerView;

    m_editButton = new IconButton(ICON_EDIT_ITEM, TR_BUTTON1);
    m_deleteButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON2);
    m_reloadButton = new IconButton(ICON_RELOAD, TR_BUTTON3);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Vertical);
    buttonBox->addButton(m_editButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_deleteButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_reloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("manage-servers");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_view);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));
    connect(m_view, SIGNAL(launched()), SLOT(handleEditServer()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(close()));
    connect(m_editButton, SIGNAL(clicked()), SLOT(handleEditServer()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDeleteServer()));
    connect(m_reloadButton, SIGNAL(clicked()), SLOT(handleReload()));

    handleSelection();
}

bool
ServersWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        g_state->store(ServersGeometryKey, saveGeometry());
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
ServersWindow::bringUp()
{
    restoreGeometry(g_state->fetch(ServersGeometryKey));
    show();
    raise();
    activateWindow();
}

void
ServersWindow::handleSelection()
{
    auto *server = m_view->selectedServer();
    m_editButton->setEnabled(server);
    m_deleteButton->setEnabled(server && !server->active());
}

void
ServersWindow::handleEditServer()
{
    auto *server = m_view->selectedServer();
    if (server)
        g_settings->serverWindow(server)->bringUp();
}

void
ServersWindow::handleDeleteServer()
{
    auto *server = m_view->selectedServer();
    if (!server || server->active())
        return;

    QString name = server->fullname();

    if (QMessageBox::Yes != askBox(TR_TITLE2, TR_ASK1.arg(name), this)->exec())
        return;

    try {
        g_settings->deleteServer(server);
    } catch (const StringException &e) {
        errBox(e.message(), this)->show();
    }
}

void
ServersWindow::handleReload()
{
    for (auto i: g_settings->servers()) {
        i->sync();
        i->loadSettings();
    }

    g_settings->rescanServers();
}
