// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/exception.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "keymapwindow.h"
#include "keymapmodel.h"
#include "keymap.h"
#include "keymapnew.h"
#include "keymapeditor.h"
#include "settings.h"
#include "profile.h"
#include "state.h"

#include <QHBoxLayout>
#include <QKeyEvent>

#define TR_ASK1 TL("question", "Really delete keymap \"%1\"?")
#define TR_BUTTON1 TL("input-button", "New Keymap") + A("...")
#define TR_BUTTON2 TL("input-button", "Clone Keymap") + A("...")
#define TR_BUTTON3 TL("input-button", "Delete Keymap")
#define TR_BUTTON4 TL("input-button", "Rename Keymap") + A("...")
#define TR_BUTTON5 TL("input-button", "Edit Keymap") + A("...")
#define TR_BUTTON6 TL("input-button", "Reload Files")
#define TR_TEXT1 TL("window-text", "Keymap is in use by profiles or other keymaps.\n")
#define TR_TITLE1 TL("window-title", "Manage Keymaps")
#define TR_TITLE2 TL("window-title", "Confirm Delete")

KeymapsWindow *g_keymapwin;

KeymapsWindow::KeymapsWindow()
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);

    m_view = new KeymapView;

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
    buttonBox->addHelpButton("manage-keymaps");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_view);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));
    connect(m_view, SIGNAL(launched()), SLOT(handleEditKeymap()));

    connect(buttonBox, SIGNAL(accepted()), SLOT(close()));
    connect(m_newButton, SIGNAL(clicked()), SLOT(handleNewKeymap()));
    connect(m_cloneButton, SIGNAL(clicked()), SLOT(handleCloneKeymap()));
    connect(m_deleteButton, SIGNAL(clicked()), SLOT(handleDeleteKeymap()));
    connect(m_renameButton, SIGNAL(clicked()), SLOT(handleRenameKeymap()));
    connect(m_editButton, SIGNAL(clicked()), SLOT(handleEditKeymap()));
    connect(m_reloadButton, SIGNAL(clicked()), SLOT(handleReload()));

    handleSelection();
}

bool
KeymapsWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        g_state->store(KeymapsGeometryKey, saveGeometry());
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
KeymapsWindow::bringUp()
{
    restoreGeometry(g_state->fetch(KeymapsGeometryKey));
    show();
    raise();
    activateWindow();
}

void
KeymapsWindow::handleSelection()
{
    auto *keymap = m_view->selectedKeymap();

    if (!keymap) {
        m_cloneButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        m_renameButton->setEnabled(false);
        m_editButton->setEnabled(false);
    } else {
        m_cloneButton->setEnabled(true);
        m_deleteButton->setEnabled(!keymap->reserved());
        m_renameButton->setEnabled(!keymap->reserved());
        m_editButton->setEnabled(true);
    }
}

void
KeymapsWindow::handleNewKeymap()
{
    NewKeymapDialog dialog(nullptr, false, this);
    TermKeymap *keymap;

    do {
        if (QDialog::Accepted != dialog.exec())
            return;

        try {
            keymap = g_settings->newKeymap(dialog.name(), dialog.parent());
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectKeymap(keymap);
    handleEditKeymap();
}

void
KeymapsWindow::handleCloneKeymap()
{
    auto *from = m_view->selectedKeymap();
    if (!from)
        return;

    NewKeymapDialog dialog(from, true, this);
    TermKeymap *to;

    do {
        if (QDialog::Accepted != dialog.exec())
            return;

        try {
            to = g_settings->cloneKeymap(from, dialog.name(), dialog.parent());
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectKeymap(to);
    handleEditKeymap();
}

void
KeymapsWindow::handleRenameKeymap()
{
    auto *from = m_view->selectedKeymap();
    if (!from)
        return;

    NewKeymapDialog dialog(from, false, this);
    TermKeymap *to;

    do {
        if (QDialog::Accepted != dialog.exec())
            return;

        try {
            to = (from->name() != dialog.name()) ?
                g_settings->renameKeymap(from, dialog.name(), dialog.parent()) :
                g_settings->reparentKeymap(from, dialog.parent());
            break;
        } catch (const StringException &e) {
            errBox(e.message(), this)->exec();
        }
    } while (1);

    m_view->selectKeymap(to);
}

void
KeymapsWindow::handleDeleteKeymap()
{
    auto *keymap = m_view->selectedKeymap();
    if (!keymap)
        return;

    QString message(TR_ASK1.arg(keymap->name()));
    if (keymap->profileCount() > 0 || keymap->keymapCount() > 1)
        message.prepend(TR_TEXT1);

    if (QMessageBox::Yes != askBox(TR_TITLE2, message, this)->exec())
        return;

    try {
        g_settings->deleteKeymap(keymap);
    } catch (const StringException &e) {
        errBox(e.message(), this)->show();
    }
}

void
KeymapsWindow::handleEditKeymap()
{
    auto *keymap = m_view->selectedKeymap();
    if (!keymap)
        return;

    g_settings->keymapWindow(keymap)->bringUp();
}

void
KeymapsWindow::handleReload()
{
    for (auto i: g_settings->keymaps())
        i->reload();

    g_settings->rescanKeymaps();
    g_settings->rescanProfiles();
}
