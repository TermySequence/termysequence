// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/exception.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "launchwindow.h"
#include "launchmodel.h"
#include "launcher.h"
#include "settings.h"
#include "settingswindow.h"
#include "state.h"

#include <QHBoxLayout>
#include <QKeyEvent>

#define TR_ASK1 TL("question", "Really delete item \"%1\"?")
#define TR_BUTTON1 TL("input-button", "New Item") + A("...")
#define TR_BUTTON2 TL("input-button", "Clone Item") + A("...")
#define TR_BUTTON3 TL("input-button", "Delete Item")
#define TR_BUTTON4 TL("input-button", "Rename Item") + A("...")
#define TR_BUTTON5 TL("input-button", "Edit Item") + A("...")
#define TR_BUTTON6 TL("input-button", "Reload Files")
#define TR_BUTTON7 TL("input-button", "Import App")
#define TR_FIELD1 TL("input-field", "New item name") + ':'
#define TR_TEXT1 TL("window-title", "Choose desktop application file to import")
#define TR_TITLE1 TL("window-title", "Manage Launchers")
#define TR_TITLE2 TL("window-title", "New Launcher")
#define TR_TITLE3 TL("window-title", "Clone Launcher")
#define TR_TITLE4 TL("window-title", "Rename Launcher")
#define TR_TITLE5 TL("window-title", "Confirm Delete")

LaunchersWindow *g_launchwin;

LaunchersWindow::LaunchersWindow()
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_view = new LauncherView;

    m_newButton = new IconButton(ICON_NEW_ITEM, TR_BUTTON1);
    m_cloneButton = new IconButton(ICON_CLONE_ITEM, TR_BUTTON2);
    m_deleteButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON3);
    m_renameButton = new IconButton(ICON_RENAME_ITEM, TR_BUTTON4);
    m_editButton = new IconButton(ICON_EDIT_ITEM, TR_BUTTON5);
    m_reloadButton = new IconButton(ICON_RELOAD, TR_BUTTON6);
    m_importButton = new IconButton(ICON_IMPORT_ITEM, TR_BUTTON7);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Vertical);
    buttonBox->addButton(m_newButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_cloneButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_deleteButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_renameButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_editButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_importButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_reloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("manage-launchers");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_view);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));
    connect(m_view, SIGNAL(launched()), SLOT(handleEditLauncher()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(close()));
    connect(m_newButton, SIGNAL(clicked()), SLOT(handleNewLauncher()));
    connect(m_cloneButton, SIGNAL(clicked()), SLOT(handleCloneLauncher()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDeleteLauncher()));
    connect(m_renameButton, SIGNAL(clicked()), SLOT(handleRenameLauncher()));
    connect(m_editButton, SIGNAL(clicked()), SLOT(handleEditLauncher()));
    connect(m_importButton, SIGNAL(clicked()), SLOT(handleImport()));
    connect(m_reloadButton, SIGNAL(clicked()), SLOT(handleReload()));

    handleSelection();
}

bool
LaunchersWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        g_state->store(LaunchersGeometryKey, saveGeometry());
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
LaunchersWindow::bringUp()
{
    restoreGeometry(g_state->fetch(LaunchersGeometryKey));
    show();
    raise();
    activateWindow();
}

void
LaunchersWindow::handleSelection()
{
    auto *launcher = m_view->selectedLauncher();

    if (!launcher) {
        m_cloneButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        m_editButton->setEnabled(false);
    } else {
        m_cloneButton->setEnabled(true);
        m_deleteButton->setEnabled(!launcher->reserved());
        m_renameButton->setEnabled(!launcher->reserved());
        m_editButton->setEnabled(true);
    }
}

void
LaunchersWindow::handleNewLauncher()
{
    bool ok;
    LaunchSettings *launcher;

    do {
        QString name = QInputDialog::getText(this, TR_TITLE2, TR_FIELD1,
                                             QLineEdit::Normal, g_mtstr, &ok);
        if (!ok)
            return;

        try {
            launcher = g_settings->newLauncher(name);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectLauncher(launcher);
    handleEditLauncher();
    handleSelection(); // reserved could have changed
}

void
LaunchersWindow::handleCloneLauncher()
{
    auto *launcher = m_view->selectedLauncher();
    if (!launcher)
        return;

    bool ok;
    QString from = launcher->name();

    do {
        QString to = QInputDialog::getText(this, TR_TITLE3, TR_FIELD1,
                                           QLineEdit::Normal, from, &ok);
        if (!ok)
            return;

        try {
            launcher = g_settings->cloneLauncher(launcher, to);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectLauncher(launcher);
    handleEditLauncher();
    handleSelection(); // reserved could have changed
}

void
LaunchersWindow::handleRenameLauncher()
{
    auto *launcher = m_view->selectedLauncher();
    if (!launcher)
        return;

    bool ok;
    QString from = launcher->name();

    do {
        QString to = QInputDialog::getText(this, TR_TITLE4, TR_FIELD1,
                                           QLineEdit::Normal, from, &ok);
        if (!ok)
            return;

        try {
            launcher = g_settings->renameLauncher(launcher, to);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectLauncher(launcher);
}

void
LaunchersWindow::handleDeleteLauncher()
{
    auto *launcher = m_view->selectedLauncher();
    if (!launcher)
        return;

    QString name = launcher->name();

    if (QMessageBox::Yes != askBox(TR_TITLE5, TR_ASK1.arg(name), this)->exec())
        return;

    try {
        g_settings->deleteLauncher(launcher);
    } catch (const StringException &e) {
        errBox(e.message(), this)->show();
    }
}

void
LaunchersWindow::handleEditLauncher()
{
    auto *launcher = m_view->selectedLauncher();
    if (launcher)
        g_settings->launcherWindow(launcher)->bringUp();
}

void
LaunchersWindow::doImport(const QString &path)
{
    QSettings app(path, QSettings::IniFormat);
    QString name = QFileInfo(path).baseName();
    LaunchSettings *launcher;

    try {
        launcher = g_settings->newLauncher(name);
    } catch (const StringException &e) {
        errBox(e.message(), this)->show();
        return;
    }

    app.beginGroup("Desktop Entry");
    launcher->setIcon(app.value("Icon").toString());
    launcher->setLaunchType(app.value("Terminal").toBool() ? LaunchLocalTerm : LaunchLocalCommand);

    QStringList cmd = app.value("Exec").toString().split(' ');
    if (!cmd.isEmpty()) {
        cmd.prepend(cmd.front());
        launcher->setCommand(cmd);
    }

    launcher->saveSettings();

    m_view->selectLauncher(launcher);
    handleEditLauncher();
    handleSelection(); // reserved could have changed
}

void
LaunchersWindow::handleImport()
{
    auto *box = openBox(TR_TEXT1, this);
    box->setNameFilter(A("*.desktop"));
    box->setDirectory(A("/usr/share/applications"));
    connect(box, &QDialog::accepted, [=]{
        doImport(box->selectedFiles().value(0));
    });
    box->show();
}

void
LaunchersWindow::handleReload()
{
    // Total reload
    g_settings->rescanLaunchers();
}
