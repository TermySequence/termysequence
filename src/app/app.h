// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QApplication;
class QSocketNotifier;
class QMessageBox;
QT_END_NAMESPACE

// #define EXITCODE_ARGPARSE 1
#define EXITCODE_SIGNAL 2
#define EXITCODE_KILLED 3
#define EXITCODE_CANCEL 4
#define EXITCODE_FAILED 5

class TermApplication final: public QObject
{
    Q_OBJECT

private:
    QApplication *m_app;
    QSocketNotifier *m_notifier;
    QMessageBox *m_box = nullptr;

    int m_timerId;
    QMetaObject::Connection m_mocReady;

private slots:
    void handleSignal();
    void handleCancel();
    void handleReady();

protected:
    void timerEvent(QTimerEvent *event);

public:
    TermApplication(QApplication *app);

    void start(int sigfd);
    void cleanup();
};
