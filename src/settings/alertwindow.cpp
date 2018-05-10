// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/exception.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "alertwindow.h"
#include "alertmodel.h"
#include "alert.h"
#include "settings.h"
#include "settingswindow.h"
#include "state.h"

#include <QHBoxLayout>
#include <QKeyEvent>

#define TR_ASK1 TL("question", "Really delete alert \"%1\"?")
#define TR_BUTTON1 TL("input-button", "New Item") + A("...")
#define TR_BUTTON2 TL("input-button", "Clone Item") + A("...")
#define TR_BUTTON3 TL("input-button", "Delete Item")
#define TR_BUTTON4 TL("input-button", "Rename Item") + A("...")
#define TR_BUTTON5 TL("input-button", "Edit Item") + A("...")
#define TR_BUTTON6 TL("input-button", "Reload Files")
#define TR_FIELD1 TL("input-field", "New alert name") + ':'
#define TR_TITLE1 TL("window-title", "Manage Alerts")
#define TR_TITLE2 TL("window-title", "New Alert")
#define TR_TITLE3 TL("window-title", "Clone Alert")
#define TR_TITLE4 TL("window-title", "Rename Alert")
#define TR_TITLE5 TL("window-title", "Confirm Delete")

AlertsWindow *g_alertwin;

AlertsWindow::AlertsWindow()
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_view = new AlertView;

    m_newButton = new IconButton(ICON_NEW_ITEM, TR_BUTTON1);
    m_cloneButton = new IconButton(ICON_CLONE_ITEM, TR_BUTTON2);
    m_deleteButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON3);
    m_renameButton = new IconButton(ICON_RENAME_ITEM, TR_BUTTON4);
    m_editButton = new IconButton(ICON_EDIT_ITEM, TR_BUTTON5);
    m_reloadButton = new IconButton(ICON_RELOAD, TR_BUTTON6);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Vertical);
    buttonBox->addButton(m_newButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_cloneButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_deleteButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_renameButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_editButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_reloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("manage-alerts");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_view);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(close()));
    connect(m_newButton, SIGNAL(clicked()), SLOT(handleNewAlert()));
    connect(m_cloneButton, SIGNAL(clicked()), SLOT(handleCloneAlert()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDeleteAlert()));
    connect(m_renameButton, SIGNAL(clicked()), SLOT(handleRenameAlert()));
    connect(m_editButton, SIGNAL(clicked()), SLOT(handleEditAlert()));
    connect(m_reloadButton, SIGNAL(clicked()), SLOT(handleReload()));

    handleSelection();
}

bool
AlertsWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        g_state->store(AlertsGeometryKey, saveGeometry());
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
AlertsWindow::bringUp()
{
    restoreGeometry(g_state->fetch(AlertsGeometryKey));
    show();
    raise();
    activateWindow();
}

void
AlertsWindow::handleSelection()
{
    auto *alert = m_view->selectedAlert();

    m_cloneButton->setEnabled(alert);
    m_deleteButton->setEnabled(alert);
    m_renameButton->setEnabled(alert);
    m_editButton->setEnabled(alert);
}

void
AlertsWindow::handleNewAlert()
{
    bool ok;
    AlertSettings *alert;

    do {
        QString name = QInputDialog::getText(this, TR_TITLE2, TR_FIELD1,
                                             QLineEdit::Normal, g_mtstr, &ok);
        if (!ok)
            return;

        try {
            alert = g_settings->newAlert(name);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectAlert(alert);
    handleEditAlert();
}

void
AlertsWindow::handleCloneAlert()
{
    auto *alert = m_view->selectedAlert();
    if (!alert)
        return;

    bool ok;
    QString from = alert->name();

    do {
        QString to = QInputDialog::getText(this, TR_TITLE3, TR_FIELD1,
                                           QLineEdit::Normal, from, &ok);
        if (!ok)
            return;

        try {
            alert = g_settings->cloneAlert(alert, to);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectAlert(alert);
    handleEditAlert();
}

void
AlertsWindow::handleRenameAlert()
{
    auto *alert = m_view->selectedAlert();
    if (!alert)
        return;

    QString from = alert->name();
    bool ok;

    do {
        QString to = QInputDialog::getText(this, TR_TITLE4, TR_FIELD1,
                                           QLineEdit::Normal, from, &ok);
        if (!ok)
            return;

        try {
            alert = g_settings->renameAlert(alert, to);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectAlert(alert);
}

void
AlertsWindow::handleDeleteAlert()
{
    auto *alert = m_view->selectedAlert();
    if (!alert)
        return;

    QString name = alert->name();

    if (QMessageBox::Yes != askBox(TR_TITLE5, TR_ASK1.arg(name), this)->exec())
        return;

    try {
        g_settings->deleteAlert(alert);
    } catch (const StringException &e) {
        errBox(e.message(), this)->show();
    }
}

void
AlertsWindow::handleEditAlert()
{
    auto *alert = m_view->selectedAlert();
    if (alert)
        g_settings->alertWindow(alert)->bringUp();
}

void
AlertsWindow::handleReload()
{
    for (auto i: g_settings->alerts()) {
        i->sync();
        i->loadSettings();
    }

    g_settings->rescanAlerts();
}
