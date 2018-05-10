// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/enums.h"
#include "sshdialog.h"
#include "connect.h"
#include "config.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define TR_CHECK1 TL("input-checkbox", "Use binary protocol encoding (adds -q flag)")
#define TR_CHECK2 TL("input-checkbox", "Allocate a local pty for password prompts")
#define TR_FIELD1 TL("input-field", "User@host destination") + ':'
#define TR_FIELD2 TL("input-field", "Optional additional SSH arguments") + ':'
#define TR_TEXT1 TL("window-text", \
    "Note: shell metacharacters, quotes, and escapes are not interpreted")
#define TR_TITLE1 TL("window-title", "SSH Connection")

SshDialog::SshDialog(QWidget *parent) :
    ConnectDialog(parent, "connect-ssh")
{
    setWindowTitle(TR_TITLE1);

    m_okButton->setEnabled(false);

    m_focusWidget = m_host = new QLineEdit;
    m_flags = new QLineEdit;
    m_raw = new QCheckBox(TR_CHECK1);
    m_raw->setChecked(true);
    m_pty = new QCheckBox(TR_CHECK2);
    m_pty->setChecked(true);

    m_mainLayout->insertWidget(0, new QLabel(TR_FIELD1));
    m_mainLayout->insertWidget(1, m_host);
    m_mainLayout->insertWidget(2, new QLabel(TR_FIELD2));
    m_mainLayout->insertWidget(3, new QLabel(TR_TEXT1));
    m_mainLayout->insertWidget(4, m_flags);
    m_mainLayout->insertWidget(5, m_raw);
    m_mainLayout->insertWidget(6, m_pty);

    connect(m_host, SIGNAL(textChanged(const QString&)), SLOT(handleTextChanged(const QString&)));
}

void
SshDialog::handleTextChanged(const QString &text)
{
    m_okButton->setEnabled(!text.isEmpty());
    m_saveName->setText(text);
}

void
SshDialog::handleAccept()
{
    if (!m_host->text().isEmpty()) {
        QStringList command;
        command += A("ssh");
        command += A("ssh");
        command += m_flags->text().split(' ', QString::SkipEmptyParts);
        command += m_raw->isChecked() ? A("-qT") : A("-T");
        command += m_host->text();
        command += SERVER_NAME;

        createInfo(m_host->text());
        m_info->setCommand(command);
        m_info->setType(Tsqt::ConnectionSsh);
        m_info->setRaw(m_raw->isChecked());
        m_info->setPty(m_pty->isChecked());
        m_info->setKeepalive(KEEPALIVE_DEFAULT);
        doAccept();
    }
}

ConnectSettings *
SshDialog::makeConnection(const QString &dest)
{
    QStringList command = { A("ssh"), A("ssh"), A("-qT"), dest, SERVER_NAME };

    auto *info = new ConnectSettings(dest);
    info->activate();
    info->setCommand(command);
    info->setType(Tsqt::ConnectionSsh);
    info->setRaw(true);
    info->setPty(true);
    info->setKeepalive(KEEPALIVE_DEFAULT);
    return info;
}
