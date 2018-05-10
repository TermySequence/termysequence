// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/enums.h"
#include "app/exception.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "base/listener.h"
#include "base/server.h"
#include "connectwindow.h"
#include "connectmodel.h"
#include "connect.h"
#include "connectnew.h"
#include "sshdialog.h"
#include "userdialog.h"
#include "containerdialog.h"
#include "otherdialog.h"
#include "batchdialog.h"
#include "settings.h"
#include "settingswindow.h"
#include "global.h"
#include "state.h"

#include <QHBoxLayout>
#include <QKeyEvent>

#define TR_ASK1 TL("question", "Really delete connection \"%1\"?")
#define TR_ASK2 TL("question", "Really disconnect connection \"%1\"?")
#define TR_BUTTON1 TL("input-button", "New Item") + A("...")
#define TR_BUTTON2 TL("input-button", "Clone Item") + A("...")
#define TR_BUTTON3 TL("input-button", "Delete Item")
#define TR_BUTTON4 TL("input-button", "Rename Item") + A("...")
#define TR_BUTTON5 TL("input-button", "Edit Item") + A("...")
#define TR_BUTTON6 TL("input-button", "Launch Item")
#define TR_BUTTON7 TL("input-button", "Disconnect")
#define TR_BUTTON8 TL("input-button", "Reload Files")
#define TR_FIELD1 TL("input-field", "New connection name") + ':'
#define TR_TITLE1 TL("window-title", "Manage Connections")
#define TR_TITLE2 TL("window-title", "Clone Connection")
#define TR_TITLE3 TL("window-title", "Rename Connection")
#define TR_TITLE4 TL("window-title", "Confirm Delete")
#define TR_TITLE5 TL("window-title", "Confirm Disconnect")

ConnectsWindow *g_connwin;

ConnectsWindow::ConnectsWindow()
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_view = new ConnectView;

    m_newButton = new IconButton(ICON_NEW_ITEM, TR_BUTTON1);
    m_cloneButton = new IconButton(ICON_CLONE_ITEM, TR_BUTTON2);
    m_deleteButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON3);
    m_renameButton = new IconButton(ICON_RENAME_ITEM, TR_BUTTON4);
    m_editButton = new IconButton(ICON_EDIT_ITEM, TR_BUTTON5);
    m_launchButton = new IconButton(ICON_CONNECTION_LAUNCH, TR_BUTTON6);
    m_stopButton = new IconButton(ICON_CONNECTION_CLOSE, TR_BUTTON7);
    m_reloadButton = new IconButton(ICON_RELOAD, TR_BUTTON8);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Vertical);
    buttonBox->addButton(m_newButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_cloneButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_deleteButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_renameButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_editButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_launchButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_stopButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_reloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("manage-connections");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_view);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));
    connect(m_view, SIGNAL(rowChanged()), SLOT(handleSelection()));
    connect(m_view, SIGNAL(launched()), SLOT(handleLaunchConn()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(close()));
    connect(m_newButton, SIGNAL(clicked()), SLOT(handleNewConn()));
    connect(m_cloneButton, SIGNAL(clicked()), SLOT(handleCloneConn()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDeleteConn()));
    connect(m_renameButton, SIGNAL(clicked()), SLOT(handleRenameConn()));
    connect(m_editButton, SIGNAL(clicked()), SLOT(handleEditConn()));
    connect(m_launchButton, SIGNAL(clicked()), SLOT(handleLaunchConn()));
    connect(m_stopButton, SIGNAL(clicked()), SLOT(handleLaunchConn()));
    connect(m_reloadButton, SIGNAL(clicked()), SLOT(handleReload()));

    handleSelection();
}

bool
ConnectsWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        g_state->store(ConnectsGeometryKey, saveGeometry());
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
ConnectsWindow::bringUp()
{
    restoreGeometry(g_state->fetch(ConnectsGeometryKey));
    show();
    raise();
    activateWindow();
}

void
ConnectsWindow::handleSelection()
{
    auto *conn = m_view->selectedConn();

    if (!conn) {
        m_cloneButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        m_editButton->setEnabled(false);
        m_launchButton->setEnabled(false);
        m_stopButton->setEnabled(false);
    } else {
        m_cloneButton->setEnabled(!conn->reserved());
        m_deleteButton->setEnabled(!conn->reserved() && !conn->anonymous());
        m_renameButton->setEnabled(!conn->reserved() && !conn->anonymous());
        m_editButton->setEnabled(!conn->reserved() && !conn->anonymous());
        m_stopButton->setEnabled(conn->active());
        m_launchButton->setEnabled(!conn->active());
    }
}

void
ConnectsWindow::handleNewConn()
{
    NewConnectDialog typeDialog(this);
    int result = typeDialog.exec();
    int type = typeDialog.type();
    typeDialog.hide();

    if (QDialog::Accepted == result)
    {
        ConnectDialog *dialog;

        switch (type) {
        case Tsqt::ConnectionBatch:
            dialog = new BatchDialog(this);
            break;
        case Tsqt::ConnectionSsh:
            dialog = new SshDialog(this);
            break;
        case Tsqt::ConnectionUserSudo:
        case Tsqt::ConnectionUserSu:
        case Tsqt::ConnectionUserMctl:
        case Tsqt::ConnectionUserPkexec:
            dialog = new UserDialog(this, type);
            break;
        case Tsqt::ConnectionMctl:
        case Tsqt::ConnectionDocker:
        case Tsqt::ConnectionKubectl:
        case Tsqt::ConnectionRkt:
            dialog = new ContainerDialog(this, type);
            break;
        default:
            dialog = new OtherDialog(this);
            break;
        }

        dialog->setNameRequired();
        connect(dialog, &ConnectDialog::saved, [this](ConnectSettings *conn) {
            m_view->selectConn(conn);
        });
        dialog->show();
    }
}

void
ConnectsWindow::handleCloneConn()
{
    auto *conn = m_view->selectedConn();
    if (!conn)
        return;

    bool ok;
    QString from = conn->name();

    do {
        QString to = QInputDialog::getText(this, TR_TITLE2, TR_FIELD1,
                                           QLineEdit::Normal, from, &ok);
        if (!ok)
            return;

        try {
            conn = g_settings->cloneConn(conn, to);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectConn(conn);
    handleEditConn();
}

void
ConnectsWindow::handleRenameConn()
{
    auto *conn = m_view->selectedConn();
    if (!conn)
        return;

    QString from = conn->name();
    bool ok;

    do {
        QString to = QInputDialog::getText(this, TR_TITLE3, TR_FIELD1,
                                           QLineEdit::Normal, from, &ok);
        if (!ok)
            return;

        try {
            conn = g_settings->renameConn(conn, to);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectConn(conn);
}

void
ConnectsWindow::handleDeleteConn()
{
    auto *conn = m_view->selectedConn();
    if (!conn)
        return;

    QString name = conn->name();

    if (QMessageBox::Yes != askBox(TR_TITLE4, TR_ASK1.arg(name), this)->exec())
        return;

    try {
        g_settings->deleteConn(conn);
    } catch (const StringException &e) {
        errBox(e.message(), this)->show();
    }
}

void
ConnectsWindow::handleEditConn()
{
    auto *conn = m_view->selectedConn();
    if (conn) {
        if (conn->isbatch()) {
            (new BatchDialog(this, conn))->show();
        } else {
            g_settings->connWindow(conn)->bringUp();
        }
    }
}

void
ConnectsWindow::handleLaunchConn()
{
    auto *conn = m_view->selectedConn();
    auto *manager = g_listener->activeManager();

    if (!conn || !manager)
        return;

    QString name = conn->name();

    if (!conn->active()) {
        g_listener->launchConnection(conn, manager);
        if (g_global->closeOnLaunch())
            close();
    }
    else if (QMessageBox::Yes == askBox(TR_TITLE5, TR_ASK2.arg(name), this)->exec())
        conn->activeServer()->unconnect();
}

void
ConnectsWindow::handleReload()
{
    for (auto i: g_settings->conns()) {
        i->sync();
        i->loadSettings();
    }

    g_settings->rescanConnections();
}
