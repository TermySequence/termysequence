// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QScrollBar;
class QPushButton;
QT_END_NAMESPACE

class LogWindow final: public QDialog
{
    Q_OBJECT

private:
    QTextEdit *m_text;
    QScrollBar *m_scroll;

    QPushButton *m_stopButton;
    QPushButton *m_resumeButton;

    bool m_blocked, m_autoscroll, m_paused;

signals:
    void logSignal(QtMsgType type, const QString msg);

private slots:
    void logSlot(QtMsgType type, const QString msg);

    void handleStop();
    void handleResume();
    void autoScroll(int state);
    void saveAs();

protected:
    bool event(QEvent *event);

public:
    LogWindow();

    QSize sizeHint() const { return QSize(640, 480); }

    void bringUp();
    bool log(QtMsgType type, const QString &msg);
};

extern LogWindow *g_logwin;
