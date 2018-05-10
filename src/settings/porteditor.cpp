// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "base/server.h"
#include "porteditor.h"
#include "settings.h"
#include "settingswindow.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QComboBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>

#define TR_CHECK1 TL("input-checkbox", "Local: listen for connections on the client")
#define TR_CHECK2 TL("input-checkbox", "Remote: listen for connections on the server")
#define TR_ERROR1 TL("error", "One or more required fields is empty")
#define TR_FIELD1 TL("input-field", "Listen type") + ':'
#define TR_FIELD2 TL("input-field", "Listen address") + ':'
#define TR_FIELD3 TL("input-field", "Listen port") + ':'
#define TR_FIELD4 TL("input-field", "Connect type") + ':'
#define TR_FIELD5 TL("input-field", "Connect address") + ':'
#define TR_FIELD6 TL("input-field", "Connect port") + ':'
#define TR_TEXT1 TL("window-text", "Forwarding type")
#define TR_TEXT2 TL("window-text", "Server") + A(": ")
#define TR_TEXT3 TL("window-text", "<a href='#'>Edit server</a> to make permanent changes")
#define TR_TEXT4 TL("window-text", "Note: use blank address to bind on all interfaces")
#define TR_TITLE1 TL("window-title", "New Port Forwarding Task")
#define TR_TITLE2 TL("window-title", "Edit Port Forwarding Rule")

PortEditor::PortEditor(QWidget *parent, bool showlink):
    QDialog(parent)
{
    setWindowTitle(TR_TITLE1);
    setWindowModality(Qt::WindowModal);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(QDialogButtonBox::Ok);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    buttonBox->addHelpButton("port-forwarding");

    m_local = new QRadioButton(TR_CHECK1);
    m_local->setChecked(true);
    m_remote = new QRadioButton(TR_CHECK2);

    QStringList types = { L("TCP socket"), L("Unix domain socket") };
    m_ltype = new QComboBox;
    m_ltype->addItems(types);
    m_ctype = new QComboBox;
    m_ctype->addItems(types);

    m_laddr = new QLineEdit;
    m_caddr = new QLineEdit;
    m_lport = new QLineEdit;
    m_cport = new QLineEdit;

    QVBoxLayout *listenLayout = new QVBoxLayout;
    listenLayout->addWidget(m_local);
    listenLayout->addWidget(m_remote);
    QGroupBox *listenGroup = new QGroupBox(TR_TEXT1);
    listenGroup->setLayout(listenLayout);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel(TR_FIELD1), 0, 0);
    layout->addWidget(m_ltype, 0, 1);
    layout->addWidget(new QLabel(TR_FIELD2), 1, 0);
    layout->addWidget(m_laddr, 1, 1);
    layout->addWidget(new QLabel(TR_FIELD3), 2, 0);
    layout->addWidget(m_lport, 2, 1);
    layout->addWidget(new QLabel(TR_FIELD4), 3, 0);
    layout->addWidget(m_ctype, 3, 1);
    layout->addWidget(new QLabel(TR_FIELD5), 4, 0);
    layout->addWidget(m_caddr, 4, 1);
    layout->addWidget(new QLabel(TR_FIELD6), 5, 0);
    layout->addWidget(m_cport, 5, 1);
    layout->setColumnStretch(1, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    if (showlink) {
        m_label = new QLabel;
        m_label->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(m_label);

        QLabel *link = new QLabel(TR_TEXT3);
        link->setAlignment(Qt::AlignCenter);
        link->setTextFormat(Qt::RichText);
        link->setContextMenuPolicy(Qt::NoContextMenu);
        mainLayout->addWidget(link);
        connect(link, &QLabel::linkActivated, this, &PortEditor::handleLink);
    }

    mainLayout->addWidget(listenGroup);
    mainLayout->addLayout(layout, 1);
    mainLayout->addWidget(new QLabel(TR_TEXT4));
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(handleAccept()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));

    connect(m_ltype, SIGNAL(currentIndexChanged(int)), SLOT(handleTypeChange()));
    connect(m_ctype, SIGNAL(currentIndexChanged(int)), SLOT(handleTypeChange()));
}

PortFwdRule
PortEditor::buildRule() const
{
    PortFwdRule result;
    if ((result.islocal = m_local->isChecked())) {
        result.ltype = (Tsq::PortForwardTaskType)m_ltype->currentIndex();
        result.laddr = m_laddr->text().toStdString();
        result.lport = m_lport->text().toStdString();
        result.rtype = (Tsq::PortForwardTaskType)m_ctype->currentIndex();
        result.raddr = m_caddr->text().toStdString();
        result.rport = m_cport->text().toStdString();
    } else {
        result.rtype = (Tsq::PortForwardTaskType)m_ltype->currentIndex();
        result.raddr = m_laddr->text().toStdString();
        result.rport = m_lport->text().toStdString();
        result.ltype = (Tsq::PortForwardTaskType)m_ctype->currentIndex();
        result.laddr = m_caddr->text().toStdString();
        result.lport = m_cport->text().toStdString();
    }

    result.isauto = false;
    result.update();
    return result;
}

void
PortEditor::setRule(const PortFwdRule &rule)
{
    setWindowTitle(TR_TITLE2);

    if (rule.islocal) {
        m_local->setChecked(true);
        m_ltype->setCurrentIndex(rule.ltype);
        m_laddr->setText(QString::fromStdString(rule.laddr));
        m_lport->setText(QString::fromStdString(rule.lport));
        m_ctype->setCurrentIndex(rule.rtype);
        m_caddr->setText(QString::fromStdString(rule.raddr));
        m_cport->setText(QString::fromStdString(rule.rport));
    } else {
        m_remote->setChecked(true);
        m_ctype->setCurrentIndex(rule.ltype);
        m_caddr->setText(QString::fromStdString(rule.laddr));
        m_cport->setText(QString::fromStdString(rule.lport));
        m_ltype->setCurrentIndex(rule.rtype);
        m_laddr->setText(QString::fromStdString(rule.raddr));
        m_lport->setText(QString::fromStdString(rule.rport));
    }
}

void
PortEditor::setServer(ServerInstance *server)
{
    m_server = server;
    m_label->setText(TR_TEXT2 + server->fullname());
}

void
PortEditor::setAdding()
{
    setWindowTitle(TR_TITLE1);
}

void
PortEditor::handleTypeChange()
{
    if (m_ltype->currentIndex() == Tsq::PortForwardTCP) {
        m_lport->setEnabled(true);
    } else {
        m_lport->clear();
        m_lport->setEnabled(false);
    }
    if (m_ctype->currentIndex() == Tsq::PortForwardTCP) {
        m_cport->setEnabled(true);
    } else {
        m_cport->clear();
        m_cport->setEnabled(false);
    }
}

void
PortEditor::handleAccept()
{
    if ((m_ltype->currentIndex() == Tsq::PortForwardTCP && m_lport->text().isEmpty()) ||
        (m_ctype->currentIndex() == Tsq::PortForwardTCP && m_cport->text().isEmpty()) ||
        (m_ltype->currentIndex() == Tsq::PortForwardUNIX && m_laddr->text().isEmpty()) ||
        (m_ctype->currentIndex() == Tsq::PortForwardUNIX && m_caddr->text().isEmpty()))
    {
        errBox(TR_ERROR1, this)->show();
        return;
    }

    // TODO hostname/IP validation, etc.
    accept();
}

void
PortEditor::handleLink()
{
    g_settings->serverWindow(m_server->serverInfo())->bringUp();
    reject();
}
