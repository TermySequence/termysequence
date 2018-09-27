// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/exception.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "connectdialog.h"
#include "serverwidget.h"
#include "connect.h"
#include "settings.h"
#include "global.h"

#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define TR_CHECK1 TL("input-checkbox", "Save connection as") + ':'
#define TR_FIELD1 TL("input-field", "Launch from") + ':'

ConnectDialog::ConnectDialog(QWidget *parent, const char *helpPage, unsigned options) :
    QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::WindowModal);
    setSizeGripEnabled(true);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    m_buttonBox->addHelpButton(helpPage);
    m_okButton = m_buttonBox->button(QDialogButtonBox::Ok);

    m_mainLayout = new QVBoxLayout;

    if (options & ShowServ) {
        m_serverCombo = new ServerCombo;

        auto *serverLayout = new QHBoxLayout;
        serverLayout->setContentsMargins(g_mtmargins);
        serverLayout->addWidget(new QLabel(TR_FIELD1));
        serverLayout->addWidget(m_serverCombo, 1);
        m_mainLayout->addLayout(serverLayout);
    }
    if (options & (OptSave|ReqSave)) {
        m_saveName = new QLineEdit;

        auto *saveLayout = new QHBoxLayout;
        saveLayout->setContentsMargins(g_mtmargins);
        if (options & ReqSave) {
            saveLayout->addWidget(new QLabel(TR_CHECK1));
        } else {
            saveLayout->addWidget(m_save = new QCheckBox(TR_CHECK1));
        }
        saveLayout->addWidget(m_saveName, 1);
        m_mainLayout->addLayout(saveLayout);
    }

    m_mainLayout->addWidget(m_buttonBox);
    setLayout(m_mainLayout);

    connect(m_buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(m_buttonBox, SIGNAL(rejected()), SLOT(reject()));

    connect(this, &QObject::destroyed, [this]{
        if (m_info)
            m_info->putReference();
    });
}

bool
ConnectDialog::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        if (m_focusWidget)
            m_focusWidget->setFocus(Qt::ActiveWindowFocusReason);
        // fallthru
    default:
        return QDialog::event(event);
    }
}

void
ConnectDialog::createInfo(const QString &defaultName)
{
    if (m_info)
        m_info->putReference();

    m_info = new ConnectSettings(!m_save || m_save->isChecked() ?
                                 m_saveName->text() :
                                 defaultName);
    m_info->activate();
    m_info->setServer(m_serverCombo->currentData().toString());
}

void
ConnectDialog::doSave(const QString &name)
{
    try {
        ConnectSettings *conn;
        conn = g_settings->newConn(name, m_info->type());
        m_info->copySettings(conn);
        conn->saveSettings();
        emit saved(conn);
        accept();
    } catch (const StringException &e) {
        m_saveName->selectAll();
        m_focusWidget = m_saveName;
        errBox(e.message(), this)->show();
    }
}

void
ConnectDialog::doAccept()
{
    if (!m_save || m_save->isChecked())
        doSave(m_info->name());
    else
        accept();
}
