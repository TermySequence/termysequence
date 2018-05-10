// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/iconbutton.h"
#include "base/manager.h"
#include "base/term.h"
#include "adjustdialog.h"
#include "global.h"

#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>

#define TR_CHECK1 TL("input-checkbox", "Apply change to all terminals using the same profile")
#define TR_TEXT1 TL("window-text", \
    "<a href='#'>Edit profile %1</a> to make permanent changes")

AdjustDialog::AdjustDialog(TermInstance *term, TermManager *manager,
                           const char *helpPage, QWidget *parent) :
    QDialog(parent),
    m_term(term),
    m_manager(manager)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::NonModal); // Avoid dimming of terminal window
    setSizeGripEnabled(true);

    m_profileName = term->profileName();
    m_profileAction = L("EditProfile|") + m_profileName;
    QLabel *link = new QLabel(TR_TEXT1.arg(m_profileName));
    link->setTextFormat(Qt::RichText);
    link->setContextMenuPolicy(Qt::NoContextMenu);

    m_allCheck = new QCheckBox(TR_CHECK1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|
                                       QDialogButtonBox::Cancel|
                                       QDialogButtonBox::Reset);
    QPushButton *reset = m_buttonBox->button(QDialogButtonBox::Reset);
    m_buttonBox->addHelpButton(helpPage);

    m_mainLayout = new QVBoxLayout;
    m_mainLayout->addWidget(link, 0, Qt::AlignCenter);
    setLayout(m_mainLayout);

    connect(m_term, SIGNAL(destroyed()), SLOT(deleteLater()));
    connect(link, SIGNAL(linkActivated(const QString&)), SLOT(handleLink()));
    connect(m_buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(m_buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(this, SIGNAL(rejected()), SLOT(handleRejected()));
    connect(reset, SIGNAL(clicked()), SLOT(handleReset()));
}

void
AdjustDialog::handleRejected()
{
    // Note: this may be overridden
}

void
AdjustDialog::handleLink()
{
    m_manager->invokeSlot(m_profileAction);
    reject();
}
