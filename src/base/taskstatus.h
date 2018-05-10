// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QProgressBar;
class QCheckBox;
class QPushButton;
class QGroupBox;
QT_END_NAMESPACE
class TermTask;
class MainWindow;

class TaskStatus final: public QDialog
{
    Q_OBJECT

private:
    TermTask *m_task;
    MainWindow *m_parent;

    QLabel *m_status;
    QLabel *m_sink;
    QLabel *m_sent, *m_received;
    QProgressBar *m_progress;
    QGroupBox *m_question;

    QCheckBox *m_check;
    QPushButton *m_cancelButton;
    QPushButton *m_fileButton;

    void updateTaskInfo();

private slots:
    void handleTaskChanged();
    void handleTaskQuestion();
    void handleTaskCancel();
    void handleTaskReady(QString file);

    void handleAnswer();
    void handleDialog(int result);

public:
    TaskStatus(TermTask *task, MainWindow *parent);
};
