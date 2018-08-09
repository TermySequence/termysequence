// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "log.h"
#include "status.h"
#include "settings.h"

#include <QTextEdit>
#include <QScrollBar>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QKeyEvent>

LogWindow::LogWindow()
{
    setWindowTitle(A("Event Log"));
    setAttribute(Qt::WA_QuitOnClose, false);

    m_text = new QTextEdit;
    m_text->setReadOnly(true);
    m_text->setLineWrapMode(QTextEdit::NoWrap);
    m_scroll = m_text->verticalScrollBar();
    m_progress = new QLabel;

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_text, 1);
    layout->addWidget(m_progress);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(g_status, &MainStatus::sendLog, this, &LogWindow::handleLog);
    connect(g_status, &MainStatus::sendProgress, this, &LogWindow::handleProgress);

    MainStatus::Logbuf logbuf = std::move(g_status->fetchLog());
    for (auto elt: logbuf)
        appendLog(elt.first, elt.second);
}

void
LogWindow::bringUp()
{
    restoreGeometry(g_settings->logGeometry());
    show();
    raise();
    activateWindow();
}

void
LogWindow::appendLog(QString message, int severity)
{
    m_text->moveCursor(QTextCursor::End);
    QString fmt;
    switch (severity) {
    case 2:
        fmt = A("<font color='red'>%1</font>");
        break;
    case 1:
        fmt = A("<font color='orange'>%1</font>");
        break;
    default:
        fmt = A("%1");
        break;
    }

    m_text->insertHtml(fmt.arg(message));
    m_text->textCursor().insertBlock();
    m_scroll->setValue(m_scroll->maximum());
}

void
LogWindow::handleLog(QString message, int severity)
{
    appendLog(message, severity);
    bringUp();
}

void
LogWindow::handleProgress(QString message)
{
    m_progress->setText(message);
    if (!message.isEmpty())
        bringUp();
}

bool
LogWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::Close:
        g_settings->setLogGeometry(saveGeometry());
        break;
    case QEvent::WindowActivate:
        m_text->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QWidget::event(event);
}
