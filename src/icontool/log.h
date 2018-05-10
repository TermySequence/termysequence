// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTextEdit;
class QScrollBar;
class QLabel;
QT_END_NAMESPACE

class LogWindow final: public QWidget
{
    Q_OBJECT

private:
    QTextEdit *m_text;
    QScrollBar *m_scroll;
    QLabel *m_progress;

    void appendLog(QString message, int severity);

protected:
    bool event(QEvent *event);

private slots:
    void handleLog(QString message, int severity);
    void handleProgress(QString message);

public:
    LogWindow();

    void bringUp();

    QSize sizeHint() const { return QSize(640, 480); }
};
