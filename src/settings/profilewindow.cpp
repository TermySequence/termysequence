// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/exception.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "profilewindow.h"
#include "profilemodel.h"
#include "profile.h"
#include "settings.h"
#include "settingswindow.h"
#include "state.h"

#include <QHBoxLayout>
#include <QKeyEvent>

#define TR_ASK1 TL("question", "Really delete profile \"%1\"?")
#define TR_BUTTON1 TL("input-button", "New Profile") + A("...")
#define TR_BUTTON2 TL("input-button", "Clone Profile") + A("...")
#define TR_BUTTON3 TL("input-button", "Delete Profile")
#define TR_BUTTON4 TL("input-button", "Rename Profile") + A("...")
#define TR_BUTTON5 TL("input-button", "Edit Profile") + A("...")
#define TR_BUTTON6 TL("input-button", "Reload Files")
#define TR_FIELD1 TL("input-field", "New profile name") + ':'
#define TR_TEXT1 TL("window-text", "Profile is in use by open or recently closed terminals.\n")
#define TR_TITLE1 TL("window-title", "Manage Profiles")
#define TR_TITLE2 TL("window-title", "New Profile")
#define TR_TITLE3 TL("window-title", "Clone Profile")
#define TR_TITLE4 TL("window-title", "Rename Profile")
#define TR_TITLE5 TL("window-title", "Confirm Delete")

ProfilesWindow *g_profilewin;

ProfilesWindow::ProfilesWindow()
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_view = new ProfileView;

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
    buttonBox->addHelpButton("manage-profiles");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_view);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));
    connect(m_view, SIGNAL(launched()), SLOT(handleEditProfile()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(close()));
    connect(m_newButton, SIGNAL(clicked()), SLOT(handleNewProfile()));
    connect(m_cloneButton, SIGNAL(clicked()), SLOT(handleCloneProfile()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDeleteProfile()));
    connect(m_renameButton, SIGNAL(clicked()), SLOT(handleRenameProfile()));
    connect(m_editButton, SIGNAL(clicked()), SLOT(handleEditProfile()));
    connect(m_reloadButton, SIGNAL(clicked()), SLOT(handleReload()));

    handleSelection();
}

bool
ProfilesWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        g_state->store(ProfilesGeometryKey, saveGeometry());
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
ProfilesWindow::bringUp()
{
    restoreGeometry(g_state->fetch(ProfilesGeometryKey));
    show();
    raise();
    activateWindow();
}

void
ProfilesWindow::handleSelection()
{
    auto *profile = m_view->selectedProfile();

    if (!profile) {
        m_cloneButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        m_editButton->setEnabled(false);
    } else {
        m_cloneButton->setEnabled(true);
        m_deleteButton->setEnabled(!profile->reserved());
        m_renameButton->setEnabled(!profile->reserved());
        m_editButton->setEnabled(true);
    }
}

void
ProfilesWindow::handleNewProfile()
{
    bool ok;
    ProfileSettings *profile;

    do {
        QString name = QInputDialog::getText(this, TR_TITLE2, TR_FIELD1,
                                             QLineEdit::Normal, g_mtstr, &ok);
        if (!ok)
            return;

        try {
            profile = g_settings->newProfile(name);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectProfile(profile);
    handleEditProfile();
}

void
ProfilesWindow::handleCloneProfile()
{
    auto *profile = m_view->selectedProfile();
    if (!profile)
        return;

    bool ok;
    QString from = profile->name();

    do {
        QString to = QInputDialog::getText(this, TR_TITLE3, TR_FIELD1,
                                           QLineEdit::Normal, from, &ok);
        if (!ok)
            return;

        try {
            profile = g_settings->cloneProfile(profile, to);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectProfile(profile);
    handleEditProfile();
}

void
ProfilesWindow::handleRenameProfile()
{
    auto *profile = m_view->selectedProfile();
    if (!profile)
        return;

    bool ok;
    QString from = profile->name();

    do {
        QString to = QInputDialog::getText(this, TR_TITLE4, TR_FIELD1,
                                           QLineEdit::Normal, from, &ok);
        if (!ok)
            return;

        try {
            profile = g_settings->renameProfile(profile, to);
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectProfile(profile);
}

void
ProfilesWindow::handleDeleteProfile()
{
    auto *profile = m_view->selectedProfile();
    if (!profile)
        return;

    QString message(TR_ASK1.arg(profile->name()));
    if (profile->refcount() > 0)
        message.prepend(TR_TEXT1);

    if (QMessageBox::Yes != askBox(TR_TITLE5, message, this)->exec())
        return;

    try {
        g_settings->deleteProfile(profile);
    } catch (const StringException &e) {
        errBox(e.message(), this)->show();
    }
}

void
ProfilesWindow::handleEditProfile()
{
    auto *profile = m_view->selectedProfile();
    if (profile)
        g_settings->profileWindow(profile)->bringUp();
}

void
ProfilesWindow::handleReload()
{
    for (auto i: g_settings->profiles()) {
        i->sync();
        i->loadSettings();
    }

    g_settings->rescanProfiles();
}
