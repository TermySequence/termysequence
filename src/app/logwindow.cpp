// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "logwindow.h"
#include "attr.h"
#include "iconbutton.h"
#include "logging.h"
#include "messagebox.h"
#include "settings/global.h"

#include <QTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QEvent>
#include <QTime>
#include <QDir>

#define TR_BUTTON1 TL("input-button", "Save As") + A("...")
#define TR_BUTTON2 TL("input-button", "Stop")
#define TR_BUTTON3 TL("input-button", "Resume")
#define TR_BUTTON4 TL("input-button", "Clear")
#define TR_CHECK1 TL("input-checkbox", "Autoscroll")
#define TR_FIELD1 TL("input-field", "Specify output file") + ':'
#define TR_TITLE1 TL("window-title", "Event Log")
#define TR_TITLE2 TL("window-title", "Save File")

LogWindow *g_logwin;

LogWindow::LogWindow() :
    m_blocked(false),
    m_autoscroll(true),
    m_paused(false)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);
    setSizeGripEnabled(true);

    m_text = new QTextEdit;
    m_text->setReadOnly(true);
    m_text->setLineWrapMode(QTextEdit::NoWrap);
    m_scroll = m_text->verticalScrollBar();

    QCheckBox *checkbox = new QCheckBox(TR_CHECK1);
    checkbox->setCheckState(m_autoscroll ? Qt::Checked : Qt::Unchecked);

    m_stopButton = new IconButton(ICON_PAUSE, TR_BUTTON2);
    m_resumeButton = new IconButton(ICON_RESUME, TR_BUTTON3);
    m_resumeButton->setEnabled(false);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    auto *saveButton = new IconButton(ICON_SAVE_AS, TR_BUTTON1);
    buttonBox->addButton(saveButton, QDialogButtonBox::ActionRole);
    auto *clearButton = new IconButton(ICON_CLEAR, TR_BUTTON4);
    buttonBox->addButton(clearButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_stopButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_resumeButton, QDialogButtonBox::ActionRole);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(g_mtmargins);
    bottomLayout->addWidget(checkbox);
    bottomLayout->addWidget(buttonBox);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_text);
    layout->addLayout(bottomLayout);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(checkbox, SIGNAL(stateChanged(int)), SLOT(autoScroll(int)));
    connect(saveButton, SIGNAL(clicked()), SLOT(saveAs()));
    connect(clearButton, SIGNAL(clicked()), m_text, SLOT(clear()));
    connect(m_stopButton, SIGNAL(clicked()), SLOT(handleStop()));
    connect(m_resumeButton, SIGNAL(clicked()), SLOT(handleResume()));

    connect(this, SIGNAL(logSignal(QtMsgType,QString)), SLOT(logSlot(QtMsgType,QString)));
}

bool
LogWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::WindowActivate:
        m_text->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QDialog::event(event);
}

void
LogWindow::bringUp()
{
    if (!m_blocked) {
        show();
        raise();
        activateWindow();
    }
}

void
LogWindow::autoScroll(int state)
{
    m_autoscroll = (state == Qt::Checked);
}

void
LogWindow::handleStop()
{
    m_paused = true;
    m_stopButton->setEnabled(false);
    m_resumeButton->setEnabled(true);
}

void
LogWindow::handleResume()
{
    m_paused = false;
    m_stopButton->setEnabled(true);
    m_resumeButton->setEnabled(false);
}

void
LogWindow::saveAs()
{
    QString fileBase(L(APP_NAME ".log"));

    QString homePath(getenv("HOME"));
    QDir homeDir(homePath);
    QString path(fileBase);
    bool ok;

    if (homePath.isEmpty() || !homeDir.exists())
        homeDir = A("/tmp");
    if (homeDir.exists())
        for (int count = 1; homeDir.exists(path); ++count)
            path = L("%1.%2").arg(fileBase).arg(count);

    m_blocked = true;

    path = QInputDialog::getText(this, TR_TITLE2, TR_FIELD1, QLineEdit::Normal,
                                 homeDir.filePath(path), &ok);
    if (ok) {
        QFile file(path);

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream out(&file);
            out << m_text->toPlainText();
            out.flush();
        } else {
            QString msg = path + A(": ") + file.errorString();
            errBox(msg, this)->show();
        }
    }

    m_blocked = false;
}

void
LogWindow::logSlot(QtMsgType type, const QString msg)
{
    if (m_paused)
        return;

    bool checkUp = false;
    m_text->moveCursor(QTextCursor::End);
    m_text->insertPlainText(QTime::currentTime().toString(L("HH:mm:ss.zzz ")));

    switch (type) {
    case QtDebugMsg:
        m_text->insertHtml(L("<font color='blue'>DEBUG</font> "));
        break;
    case QtInfoMsg:
        m_text->insertHtml(L("INFO "));
        break;
    case QtWarningMsg:
        m_text->insertHtml(L("<font color='orange'>WARNING</font> "));
        checkUp = true;
        break;
    case QtCriticalMsg:
        m_text->insertHtml(L("<font color='red'>CRITICAL</font> "));
        checkUp = true;
        break;
    case QtFatalMsg:
        m_text->insertHtml(L("<font color='purple'>FATAL</font> "));
        checkUp = true;
        break;
    }

    m_text->insertPlainText(msg);
    m_text->textCursor().insertBlock();

    if (m_autoscroll)
        m_scroll->setValue(m_scroll->maximum());

    if (checkUp && type >= g_global->logThreshold())
        bringUp();
}

bool
LogWindow::log(QtMsgType type, const QString &msg)
{
    emit logSignal(type, msg);

    return g_global->logToSystem();
}
