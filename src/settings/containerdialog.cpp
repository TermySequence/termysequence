// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/enums.h"
#include "containerdialog.h"
#include "connect.h"
#include "choicewidget.h"
#include "config.h"
#include "base/thumbicon.h"

#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

#define TR_FIELD1 TL("input-field", "Connection type") + ':'
#define TR_FIELD2 TL("input-field", "Pod name") + ':'
#define TR_FIELD3 TL("input-field", "Container name") + ':'
#define TR_TITLE1 TL("window-title", "Container Connection")

ContainerDialog::ContainerDialog(QWidget *parent, int type, unsigned options) :
    ConnectDialog(parent, "connect-container", options|ShowServ|OptSave)
{
    setWindowTitle(TR_TITLE1);

    m_okButton->setEnabled(false);

    m_combo = new QComboBox;

    // Note: Must be in ascending order
    int types[] = { Tsqt::ConnectionMctl, Tsqt::ConnectionDocker,
                    Tsqt::ConnectionKubectl, Tsqt::ConnectionRkt };

    const auto *ptr = ConnectSettings::g_typeDesc;
    while ((++ptr)->value.toInt() != types[0]);

    for (int i = 0; i < sizeof(types)/sizeof(*types); ++i, ++ptr) {
        m_combo->addItem(ThumbIcon::fromTheme(ptr->icon),
                         QCoreApplication::translate("settings-enum", ptr->description),
                         ptr->value);
        if (ptr->value == type)
            m_combo->setCurrentIndex(i);
    }

    m_mainLayout->insertWidget(0, new QLabel(TR_FIELD1));
    m_mainLayout->insertWidget(1, m_combo);
    m_mainLayout->insertWidget(2, m_label1 = new QLabel(TR_FIELD2));
    m_mainLayout->insertWidget(3, m_id1 = new QLineEdit);
    m_mainLayout->insertWidget(4, new QLabel(TR_FIELD3));
    m_mainLayout->insertWidget(5, m_id2 = new QLineEdit);

    connect(m_combo, SIGNAL(currentIndexChanged(int)), SLOT(handleIndexChanged()));
    connect(m_id1, SIGNAL(textChanged(const QString&)), SLOT(handleTextChanged()));
    connect(m_id2, SIGNAL(textChanged(const QString&)), SLOT(handleTextChanged()));

    handleIndexChanged();
}

void
ContainerDialog::handleIndexChanged()
{
    bool havepod = false;

    switch (m_combo->currentData().toInt()) {
    case Tsqt::ConnectionKubectl:
    case Tsqt::ConnectionRkt:
        havepod = true;
    }

    m_focusWidget = havepod ? m_id1 : m_id2;
    m_label1->setEnabled(havepod);
    m_id1->setEnabled(havepod);
    if (!havepod)
        m_id1->clear();
}

void
ContainerDialog::handleTextChanged()
{
    QStringList list;
    if (!m_id1->text().isEmpty())
        list += m_id1->text();
    if (!m_id2->text().isEmpty())
        list += m_id2->text();

    QString text = list.join('-');
    m_okButton->setEnabled(!text.isEmpty());
    m_saveName->setText(text);
}

void
ContainerDialog::populateInfo(int type, const QString &id, ConnectSettings *info)
{
    QStringList command;
    bool raw = true;
    QString id1 = id.left(id.indexOf('\x1f'));

    switch (type) {
    default:
        command = QStringList({"machinectl", "machinectl", "shell",
                    id1, "/bin/sh", "-c"});
        raw = false;
        break;
    case Tsqt::ConnectionDocker:
        command = QStringList({"docker", "docker", "exec", "-i", id1});
        break;
    case Tsqt::ConnectionKubectl:
        // id2 is container name
        command = QStringList({"kubectl", "kubectl", "exec", "-i", id1});
        if (id.contains('\x1f')) {
            command += "-c";
            command += id.section('\x1f', 1, 1);
        }
        break;
    case Tsqt::ConnectionRkt:
        // id2 is app name
        command = QStringList({"rkt", "rkt", "enter"});
        if (id.contains('\x1f')) {
            command.append(A("--app=") + id.section('\x1f', 1, 1));
        }
        command += id1;
        break;
    }

    command += SERVER_NAME;
    info->setCommand(command);
    info->setType(type);
    info->setPty(true);
    info->setRaw(raw);
    info->setKeepalive(KEEPALIVE_DEFAULT);
}

void
ContainerDialog::handleAccept()
{
    QStringList list;
    if (!m_id1->text().isEmpty())
        list += m_id1->text();
    if (!m_id2->text().isEmpty())
        list += m_id2->text();

    QString id = list.join('\x1f');

    if (!id.isEmpty()) {
        createInfo(list.join('-'));
        populateInfo(m_combo->currentData().toInt(), id, m_info);
        doAccept();
    }
}

ConnectSettings *
ContainerDialog::makeConnection(int type, const QString &id)
{
    auto *info = new ConnectSettings(id);
    info->activate();
    populateInfo(type, id, info);
    return info;
}
