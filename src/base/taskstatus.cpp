// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/iconbutton.h"
#include "app/messagebox.h"
#include "taskstatus.h"
#include "task.h"
#include "mainwindow.h"
#include "settings/global.h"
#include "lib/enums.h"

#include <QLabel>
#include <QProgressBar>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMenu>
#include <QStyle>
#include <QTimer>

#define TR_ASK1 TL("question", "Really cancel this task?")
#define TR_BUTTON1 TL("input-button", "Cancel")
#define TR_BUTTON2 TL("input-button", "Rename")
#define TR_BUTTON3 TL("input-button", "Overwrite")
#define TR_BUTTON4 TL("input-button", "Yes")
#define TR_BUTTON5 TL("input-button", "File")
#define TR_CHECK1 TL("input-checkbox", "Close when finished")
#define TR_FIELD1 TL("input-field", "From") + ':'
#define TR_FIELD2 TL("input-field", "Source") + ':'
#define TR_FIELD3 TL("input-field", "To") + ':'
#define TR_FIELD4 TL("input-field", "Destination") + ':'
#define TR_FIELD5 TL("input-field", "Bytes sent") + ':'
#define TR_FIELD6 TL("input-field", "Bytes received") + ':'
#define TR_FIELD7 TL("input-field", "Status") + ':'
#define TR_FIELD8 TL("input-field", "Type") + ':'
#define TR_TEXT1 TL("window-text", "Question")
#define TR_TITLE1 TL("window-title", "Task Status")
#define TR_TITLE2 TL("window-title", "Confirm Cancel")

#define OBJPROP_ANSWER "taskAnswer"

TaskStatus::TaskStatus(TermTask *task, MainWindow *parent) :
    QDialog(parent),
    m_task(task),
    m_parent(parent)
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowModality(Qt::NonModal);
    setSizeGripEnabled(true);

    m_progress = new QProgressBar;
    m_progress->setMinimum(0);
    m_progress->setMaximum(100);
    m_progress->setValue(task->progress());
    m_progress->setTextVisible(false);

    m_question = new QGroupBox(TR_TEXT1);
    m_check = new QCheckBox(TR_CHECK1);
    m_cancelButton = new IconButton(ICON_CANCEL_TASK, TR_BUTTON1);
    m_fileButton = new IconButton(ICON_OPEN_FILE, TR_BUTTON5);
    m_fileButton->setMenu(new QMenu(this));
    m_status = new QLabel;
    m_status->setTextFormat(Qt::RichText);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    buttonBox->addButton(m_fileButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_cancelButton, QDialogButtonBox::ActionRole);

    auto *typeLayout = new QHBoxLayout;
    if (!task->typeIcon().isNull()) {
        auto *typeLabel = new QLabel;
        typeLabel->setPixmap(task->typeIcon().pixmap(m_check->sizeHint().height()));
        typeLayout->addWidget(typeLabel);
    }
    typeLayout->addWidget(new QLabel(task->typeStr()), 1);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel(TR_FIELD8), 0, 0);
    layout->addLayout(typeLayout, 0, 1);
    layout->addWidget(new QLabel(TR_FIELD1), 1, 0);
    layout->addWidget(new QLabel(task->fromStr()), 1, 1);
    layout->addWidget(new QLabel(TR_FIELD2), 2, 0);
    layout->addWidget(new QLabel(task->sourceStr()), 2, 1);
    layout->addWidget(new QLabel(TR_FIELD3), 3, 0);
    layout->addWidget(new QLabel(task->toStr()), 3, 1);
    layout->addWidget(new QLabel(TR_FIELD4), 4, 0);
    layout->addWidget(m_sink = new QLabel, 4, 1);
    layout->addWidget(new QLabel(TR_FIELD5), 5, 0);
    layout->addWidget(m_sent = new QLabel, 5, 1);
    layout->addWidget(new QLabel(TR_FIELD6), 6, 0);
    layout->addWidget(m_received = new QLabel, 6, 1);
    layout->addWidget(new QLabel(TR_FIELD7), 7, 0);
    layout->addWidget(m_status, 7, 1);
    layout->setColumnStretch(1, 1);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_check, 1);
    buttonLayout->addWidget(buttonBox);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_question);
    mainLayout->addLayout(layout, 1);
    mainLayout->addWidget(m_progress);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));

    if (!task->finished()) {
        connect(task, SIGNAL(taskChanged()), SLOT(handleTaskChanged()));
        connect(task, SIGNAL(taskQuestion()), SLOT(handleTaskQuestion()));
        connect(task, SIGNAL(ready(QString)), SLOT(handleTaskReady(QString)));
        m_check->setChecked(g_global->autoHideTask());
        connect(m_cancelButton, SIGNAL(clicked()), SLOT(handleTaskCancel()));
    } else {
        m_check->setEnabled(false);
        m_cancelButton->setEnabled(false);
    }

    updateTaskInfo();
    handleTaskReady(task->launchfile());
    handleTaskQuestion();

    task->setDialog(this);
}

void
TaskStatus::updateTaskInfo()
{
    QString status = m_task->statusStr().toHtmlEscaped();
    if (m_task->finished()) {
        QChar c = m_task->succeeded() ? 0x2714 : 0x2718;
        const QColor &color = g_global->color(m_task->finishColor());
        status.prepend(L("<font style='color:%1'>%2</font> ")
                       .arg(color.name(QColor::HexRgb), c));
    }

    m_status->setText(status);
    m_sink->setText(m_task->sinkStr());
    m_sent->setText(QString::number(m_task->sent()));
    m_received->setText(QString::number(m_task->received()));

    m_progress->setValue(m_task->progress());
    m_progress->setVisible(m_task->progress() != -2);
}

void
TaskStatus::handleTaskReady(QString file)
{
    bool ready = !file.isEmpty();

    if (m_fileButton->isEnabled() != ready)
    {
        m_fileButton->setEnabled(ready);
        if (ready) {
            // Grab a popup menu and disable DOH
            auto *menu = m_parent->getTaskPopup(this, m_task, true);
            disconnect(menu, SIGNAL(aboutToHide()), menu, SLOT(deleteLater()));
            // connect(menu, SIGNAL(triggered(QAction*)), SLOT(accept()));
            m_fileButton->menu()->deleteLater();
            m_fileButton->setMenu(menu);
        }
    }
}

void
TaskStatus::handleTaskChanged()
{
    updateTaskInfo();

    if (m_task->finished()) {
        m_task->disconnect(this);
        m_cancelButton->disconnect(this);

        m_check->setEnabled(false);
        m_cancelButton->setEnabled(false);
        handleTaskReady(m_task->launchfile());

        if (m_task->questionCancel()) {
            accept();
        } else {
            if (m_check->isChecked())
                QTimer::singleShot(g_global->taskTime() / 2, this, SLOT(accept()));

            adjustSize();
        }
    }
}

void
TaskStatus::handleTaskQuestion()
{
    if (m_task->questioning()) {
        QDialogButtonBox *questionBox = new QDialogButtonBox;
        QPushButton *button;

        switch (m_task->questionType()) {
        case Tsq::TaskOverwriteRenameQuestion:
            button = new QPushButton(TR_BUTTON2);
            button->setProperty(OBJPROP_ANSWER, Tsq::TaskRename);
            connect(button, SIGNAL(clicked()), SLOT(handleAnswer()));
            questionBox->addButton(button, QDialogButtonBox::ActionRole);

            button = new QPushButton(TR_BUTTON3);
            button->setProperty(OBJPROP_ANSWER, Tsq::TaskOverwrite);
            connect(button, SIGNAL(clicked()), SLOT(handleAnswer()));
            questionBox->addButton(button, QDialogButtonBox::ActionRole);
            break;
        default:
            button = new QPushButton(TR_BUTTON4);
            button->setProperty(OBJPROP_ANSWER, Tsq::TaskOverwrite);
            connect(button, SIGNAL(clicked()), SLOT(handleAnswer()));
            questionBox->addButton(button, QDialogButtonBox::ActionRole);
            break;
        }

        QIcon icon = style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, this);
        int dim = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);
        QLabel *iconLabel = new QLabel;
        if (!icon.isNull())
            iconLabel->setPixmap(icon.pixmap(dim, dim));

        QHBoxLayout *questionLayout = new QHBoxLayout;
        questionLayout->addWidget(iconLabel);
        questionLayout->addWidget(new QLabel(m_task->questionStr()), 1);
        questionLayout->addWidget(questionBox);
        delete m_question->layout();
        m_question->setLayout(questionLayout);
        m_question->setVisible(true);

        show();
        raise();
    } else {
        m_question->setVisible(false);
        adjustSize();
    }
}

void
TaskStatus::handleTaskCancel()
{
    if (m_task->questioning()) {
        handleDialog(QMessageBox::Yes);
    } else {
        auto *box = askBox(TR_TITLE2, TR_ASK1, this);
        connect(box, SIGNAL(finished(int)), SLOT(handleDialog(int)));
        box->show();
    }
}

void
TaskStatus::handleAnswer()
{
    m_task->handleAnswer(sender()->property(OBJPROP_ANSWER).toInt());
}

void
TaskStatus::handleDialog(int result)
{
    if (result == QMessageBox::Yes)
        QTimer::singleShot(0, m_task, &TermTask::cancel);
}
