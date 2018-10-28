// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/enums.h"
#include "userdialog.h"
#include "connect.h"
#include "choicewidget.h"
#include "serverwidget.h"
#include "base/thumbicon.h"

#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define TR_FIELD1 TL("input-field", "Connection type") + ':'
#define TR_FIELD2 TL("input-field", "Username") + ':'
#define TR_TITLE1 TL("window-title", "Local User Connection")

UserDialog::UserDialog(QWidget *parent, int type, unsigned options) :
    ConnectDialog(parent, "connect-user", options|ShowServ|OptSave)
{
    setWindowTitle(TR_TITLE1);

    m_okButton->setEnabled(false);

    m_focusWidget = m_user = new QLineEdit;
    m_combo = new QComboBox;

    // Note: Must be in ascending order
    int types[] = { Tsqt::ConnectionUserSudo, Tsqt::ConnectionUserSu,
                    Tsqt::ConnectionUserMctl, Tsqt::ConnectionUserPkexec };

    const auto *ptr = ConnectSettings::g_typeDesc;
    while ((++ptr)->value.toInt() != types[0]);

    for (int i = 0; i < ARRAY_SIZE(types); ++i, ++ptr) {
        m_combo->addItem(ThumbIcon::fromTheme(ptr->icon),
                         QCoreApplication::translate("settings-enum", ptr->description),
                         ptr->value);
        if (ptr->value == type)
            m_combo->setCurrentIndex(i);
    }

    m_mainLayout->insertWidget(0, new QLabel(TR_FIELD1));
    m_mainLayout->insertWidget(1, m_combo);
    m_mainLayout->insertWidget(2, new QLabel(TR_FIELD2));
    m_mainLayout->insertWidget(3, m_user);

    connect(m_user, SIGNAL(textChanged(const QString&)), SLOT(handleTextChanged(const QString&)));
    connect(m_serverCombo, SIGNAL(currentIndexChanged(int)), SLOT(handleServerChanged()));
}

void
UserDialog::handleTextChanged(const QString &text)
{
    QString host = m_serverCombo->currentHost();
    m_okButton->setEnabled(!text.isEmpty());
    m_saveName->setText(text.isEmpty() ? text : text + '@' + host);
}

void
UserDialog::handleServerChanged()
{
    QString text = m_user->text();
    QString host = m_serverCombo->currentHost();
    m_saveName->setText(text.isEmpty() ? text : text + '@' + host);
}

void
UserDialog::populateInfo(int type, const QString &user, ConnectSettings *info)
{
    QStringList command;
    bool raw = true, pty = false;

    switch (type) {
    default:
        command = QStringList({"sudo", "sudo", "-Si", "-u", user, SERVER_NAME});
        break;
    case Tsqt::ConnectionUserSu:
        command = QStringList({"su", "su", "-l", "-c", SERVER_NAME, user});
        pty = SU_NEEDS_PTY;
        break;
    case Tsqt::ConnectionUserMctl:
        command = QStringList({"machinectl", "machinectl", "shell",
                    user + "@.host", "/bin/sh", "-c", SERVER_NAME});
        raw = false;
        break;
    case Tsqt::ConnectionUserPkexec:
        command = QStringList({"pkexec", "pkexec", "--user", user, SERVER_NAME});
        break;
    }

    info->setCommand(command);
    info->setType(type);
    info->setPty(pty);
    info->setRaw(raw);
}

void
UserDialog::handleAccept()
{
    if (!m_user->text().isEmpty()) {
        QString user = m_user->text();
        createInfo(user + '@' + m_serverCombo->currentHost());
        populateInfo(m_combo->currentData().toInt(), user, m_info);
        doAccept();
    }
}

ConnectSettings *
UserDialog::makeConnection(int type, const QString &user)
{
    auto *info = new ConnectSettings(user + A("@localhost"));
    info->activate();
    populateInfo(type, user, info);
    return info;
}
