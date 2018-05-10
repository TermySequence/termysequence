// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "app/iconbutton.h"
#include "connectstatus.h"
#include "listener.h"
#include "server.h"
#include "settings/state.h"
#include "settings/connect.h"
#include "settings/launcher.h"

#include <QFontDatabase>
#include <QLabel>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QTimer>

#define TR_BUTTON1 TL("input-button", "Enter")
#define TR_BUTTON2 TL("input-button", "Retry")
#define TR_BUTTON3 TL("input-button", "Scroll to Top")
#define TR_FIELD1 TL("input-field", "Password, pass phrase, or other input") + ':'
#define TR_TEXT1 TL("window-text", "Waiting for connection") + A("...")
#define TR_TEXT2 TL("window-text", \
    "Note: Ensure that %1 is in the user's PATH on the target system")
#define TR_TEXT3 TL("window-text", "Launching Connection")
#define TR_TEXT4 TL("window-text", "On Server")
#define TR_TEXT5 TL("window-text", "Launching")
#define TR_TEXT6 TL("window-text", "Waiting for task start") + A("...")
#define TR_TEXT7 TL("window-text", "PID %1 is running")
#define TR_TEXT8 TL("window-text", "PID %1 finished")
#define TR_TITLE1 TL("window-title", "Connection Status: %1")
#define TR_TITLE2 TL("window-title", "Command Output: %1")

//
// Connection status
//
ConnectStatusDialog::ConnectStatusDialog(QWidget *parent, ConnectSettings *info) :
    QDialog(parent)
{
    setWindowTitle(TR_TITLE1.arg(info->name()));
    setAttribute(Qt::WA_DeleteOnClose, true);
    setSizeGripEnabled(true);

    (m_conninfo = info)->takeReference();

    m_output = new QPlainTextEdit;
    m_output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_output->setReadOnly(true);
    m_output->setFocusPolicy(Qt::NoFocus);
    m_input = new QLineEdit;
    m_input->setEchoMode(QLineEdit::Password);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    m_inputButton = buttonBox->addButton(TR_BUTTON1, QDialogButtonBox::AcceptRole);
    m_restartButton = buttonBox->addButton(TR_BUTTON2, QDialogButtonBox::ActionRole);
    m_restartButton->setEnabled(false);
    buttonBox->addHelpButton("connection-status");

    m_icon = new QLabel;
    setIcon(QStyle::SP_MessageBoxInformation);
    m_status = new QLabel(TR_TEXT1);

    QString str = A("<b>%1</b>: ");
    QString msg = str.arg(TR_TEXT3) + info->name().toHtmlEscaped();
    if (!info->launchName().isEmpty())
        msg += ' ' + str.arg(TR_TEXT4) + info->launchName().toHtmlEscaped();
    auto *label = new QLabel(msg);
    label->setTextFormat(Qt::RichText);

    QVBoxLayout *statusLayout = new QVBoxLayout;
    statusLayout->setContentsMargins(g_mtmargins);
    statusLayout->setSpacing(0);
    statusLayout->addWidget(label);
    statusLayout->addWidget(m_status);
    QHBoxLayout *iconLayout = new QHBoxLayout;
    iconLayout->addWidget(m_icon);
    iconLayout->addLayout(statusLayout, 1);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(iconLayout);
    layout->addWidget(m_output, 1);
    layout->addWidget(new QLabel(TR_FIELD1));
    layout->addWidget(m_input);

    layout->addWidget(new QLabel(TR_TEXT2.arg(SERVER_NAME)));
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(handleInput()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    connect(m_restartButton, SIGNAL(clicked()), SLOT(handleRestart()));
}

ConnectStatusDialog::~ConnectStatusDialog()
{
    m_conninfo->putReference();
}

void
ConnectStatusDialog::bringUp()
{
    QTimer::singleShot(CONNECT_TIME, this, SLOT(show()));
}

void
ConnectStatusDialog::setIcon(int standardIcon)
{
    QStyle::StandardPixmap si = (QStyle::StandardPixmap)standardIcon;
    QIcon icon = style()->standardIcon(si, 0, this);
    int dim = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);

    if (icon.isNull())
        m_icon->clear();
    else
        m_icon->setPixmap(icon.pixmap(dim, dim));
}

void
ConnectStatusDialog::handleInput()
{
    QString text = m_input->text();
    emit inputRequest(text + '\n');
    m_input->clear();

    QString stars = QString('*').repeated(4 * !text.isEmpty()) + '\n';
    m_output->insertPlainText(stars);
    m_output->ensureCursorVisible();
    m_input->setFocus(Qt::OtherFocusReason);
}

void
ConnectStatusDialog::handleRestart()
{
    auto *manager = g_listener->activeManager();
    if (manager)
        g_listener->launchConnection(m_conninfo, manager);

    accept();
}

void
ConnectStatusDialog::appendOutput(const QString &output)
{
    m_output->insertPlainText(output);
    m_output->ensureCursorVisible();
}

void
ConnectStatusDialog::setFailure(const QString &status)
{
    m_input->setEnabled(false);
    m_inputButton->setEnabled(false);
    m_restartButton->setEnabled(true);
    m_status->setText(status);
    setIcon(QStyle::SP_MessageBoxCritical);
}

//
// Command status
//
CommandStatusDialog::CommandStatusDialog(LaunchSettings *launcher, ServerInstance *server)
{
    setWindowTitle(TR_TITLE2.arg(launcher->name()));
    setAttribute(Qt::WA_DeleteOnClose, true);

    m_output = new QPlainTextEdit;
    m_output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_output->setReadOnly(true);
    m_output->setFocusPolicy(Qt::NoFocus);
    m_output->insertPlainText(launcher->commandStr() + A("\n\n"));

    auto *scrollButton = new IconButton(ICON_MOVE_TOP, TR_BUTTON3);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    buttonBox->addButton(scrollButton, QDialogButtonBox::ActionRole);

    auto *icon = new QLabel;
    int dim = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);
    icon->setPixmap(launcher->nameIcon().pixmap(dim, dim));

    QString str = A("<b>%1</b>: ");
    QString msg = str.arg(TR_TEXT5) + launcher->name().toHtmlEscaped();
    if (server)
        msg += ' ' + str.arg(TR_TEXT4) + server->shortname().toHtmlEscaped();
    auto *label = new QLabel(msg);
    label->setTextFormat(Qt::RichText);

    QVBoxLayout *statusLayout = new QVBoxLayout;
    statusLayout->setContentsMargins(g_mtmargins);
    statusLayout->setSpacing(0);
    statusLayout->addWidget(label);
    statusLayout->addWidget(m_status = new QLabel(TR_TEXT6));
    QHBoxLayout *iconLayout = new QHBoxLayout;
    iconLayout->addWidget(icon);
    iconLayout->addLayout(statusLayout, 1);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(iconLayout);
    layout->addWidget(m_output, 1);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(scrollButton, SIGNAL(clicked()), SLOT(scrollRequest()));
    connect(buttonBox, SIGNAL(rejected()), SLOT(close()));
}

bool
CommandStatusDialog::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::Close:
        g_state->store(OutputGeometryKey, saveGeometry());
        break;
    case QEvent::WindowActivate:
        m_output->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QWidget::event(event);
}

void
CommandStatusDialog::bringUp()
{
    restoreGeometry(g_state->fetch(OutputGeometryKey));
    show();
    raise();
}

void
CommandStatusDialog::appendOutput(const QString &output)
{
    m_output->insertPlainText(output);
    m_output->ensureCursorVisible();
}

void
CommandStatusDialog::setPid(int pid)
{
    m_pid = pid;
    m_status->setText(TR_TEXT7.arg(pid) + A("..."));
}

void
CommandStatusDialog::setFinished()
{
    m_status->setText(TR_TEXT8.arg(m_pid));
}

void
CommandStatusDialog::setFailure(const QString &msg)
{
    m_status->setText(msg);
}

void
CommandStatusDialog::scrollRequest()
{
    QTextCursor tc(m_output->document());
    tc.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1);
    m_output->setTextCursor(tc);
    m_output->setFocus(Qt::OtherFocusReason);
}
