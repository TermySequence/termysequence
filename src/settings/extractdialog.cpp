// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/exception.h"
#include "app/messagebox.h"
#include "base/term.h"
#include "base/manager.h"
#include "base/mainwindow.h"
#include "extractdialog.h"
#include "settings.h"
#include "profile.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#define TR_ASK1 TL("question", "Profile \"%1\" exists. Overwrite?")
#define TR_CHECK1 TL("input-checkbox", "Review profile after extraction (recommended)")
#define TR_CHECK2 TL("input-checkbox", "Scan other terminals for this profile name")
#define TR_FIELD1 TL("input-field", "New profile name") + ':'
#define TR_TITLE1 TL("window-title", "Extract Profile")
#define TR_TITLE2 TL("window-title", "Confirm Overwrite")

ExtractDialog::ExtractDialog(TermInstance *term, TermManager *manager):
    QDialog(manager->parent()),
    m_term(term),
    m_manager(manager),
    m_overwriteOk(false)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::WindowModal);

    m_nameField = new QLineEdit;
    populateName();
    m_reviewCheck = new QCheckBox(TR_CHECK1);
    m_reviewCheck->setChecked(true);
    m_switchCheck = new QCheckBox(TR_CHECK2);
    m_switchCheck->setChecked(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_nameField);
    layout->addWidget(m_reviewCheck);
    layout->addWidget(m_switchCheck);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(m_term, SIGNAL(destroyed()), SLOT(deleteLater()));
    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

void
ExtractDialog::populateName()
{
    QString name = m_term->attributes().value(g_attr_PROFILE, g_str_unknown);
    QString candidate = name;
    QString suffix(A(" (%1)"));
    int number = 1;

    while (g_settings->haveProfile(candidate)) {
        candidate = name + suffix.arg(number++);
    }

    m_nameField->setText(candidate);
}

void
ExtractDialog::handleDialog(int result)
{
    if (result == QMessageBox::Yes) {
        m_overwriteOk = true;
        handleAccept();
    }
}

void
ExtractDialog::handleAccept()
{
    bool exists;
    QString name = m_nameField->text(), msg;
    ProfileSettings *profile;

    try {
        exists = g_settings->validateProfileName(name, false);
    } catch (const StringException &e) {
        msg = e.message();
        goto err;
    }

    if (!exists) {
        try {
            profile = g_settings->newProfile(name);
        } catch (const StringException &e) {
            msg = e.message();
            goto err;
        }
    } else if (!m_overwriteOk) {
        auto *box = askBox(TR_TITLE2, TR_ASK1.arg(name), this);
        connect(box, SIGNAL(finished(int)), SLOT(handleDialog(int)));
        box->show();
        return;
    } else {
        profile = g_settings->profile(name);
    }

    profile->activate();
    profile->fromAttributes(m_term->attributes());

    // check keymap
    if (!g_settings->haveKeymap(profile->keymapName()))
        profile->setKeymapName(g_str_DEFAULT_KEYMAP);

    profile->saveSettings();

    if (m_switchCheck->isChecked() && !exists) {
        // Switch any terms over to the new profile
        for (auto term: m_manager->terms())
            if (term->attributes().value(g_attr_PROFILE) == name)
                term->setProfile(profile);
    }
    if (m_reviewCheck->isChecked()) {
        m_manager->actionEditProfile(name);
    }

    accept();
    return;
err:
    errBox(msg, this)->show();
}
