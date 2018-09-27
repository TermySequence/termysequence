// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "otherdialog.h"
#include "connect.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define TR_CHECK1 TL("input-checkbox", "Use binary protocol encoding")
#define TR_CHECK2 TL("input-checkbox", "Allocate a local pty for password prompts")
#define TR_DIM0 TL("settings-dimension", "Disabled", "value")
#define TR_DIM1 TL("settings-dimension", "ms")
#define TR_FIELD1 TL("input-field", "Command to run %1 on the target system")
#define TR_FIELD2 TL("input-field", "Keep-alive timeout") + ':'
#define TR_TEXT1 TL("window-text", \
    "Note: shell metacharacters, quotes, and escapes are not interpreted")
#define TR_TITLE1 TL("window-title", "Custom Connection")

OtherDialog::OtherDialog(QWidget *parent, unsigned options) :
    ConnectDialog(parent, "connect-other", options|ShowServ|OptSave)
{
    setWindowTitle(TR_TITLE1);

    m_okButton->setEnabled(false);

    m_focusWidget = m_command = new QLineEdit;
    m_raw = new QCheckBox(TR_CHECK1);
    m_raw->setChecked(true);
    m_pty = new QCheckBox(TR_CHECK2);
    m_pty->setChecked(true);
    m_keepalive = new QSpinBox;
    m_keepalive->setRange(0, 86400000);
    m_keepalive->setSingleStep(1000);
    m_keepalive->setSuffix(TR_DIM1);
    m_keepalive->setSpecialValueText(TR_DIM0);

    auto spinLayout = new QHBoxLayout;
    spinLayout->setContentsMargins(g_mtmargins);
    spinLayout->addWidget(new QLabel(TR_FIELD2));
    spinLayout->addWidget(m_keepalive, 1);

    m_mainLayout->insertWidget(0, new QLabel(TR_FIELD1.arg(SERVER_NAME) + ':'));
    m_mainLayout->insertWidget(1, new QLabel(TR_TEXT1));
    m_mainLayout->insertWidget(2, m_command);
    m_mainLayout->insertWidget(3, m_raw);
    m_mainLayout->insertWidget(4, m_pty);
    m_mainLayout->insertLayout(5, spinLayout);

    connect(m_command, SIGNAL(textChanged(const QString&)),
            SLOT(handleTextChanged(const QString&)));
}

void
OtherDialog::handleTextChanged(const QString &text)
{
    m_okButton->setEnabled(!text.isEmpty());
}

void
OtherDialog::handleAccept()
{
    auto command = m_command->text().split(' ', QString::SkipEmptyParts);

    if (!command.isEmpty()) {
        command.insert(1, command[0]);
        createInfo(command[0]);
        m_info->setCommand(command);
        m_info->setRaw(m_raw->isChecked());
        m_info->setPty(m_pty->isChecked());
        m_info->setKeepalive(m_keepalive->value());
        doAccept();
    }
}
