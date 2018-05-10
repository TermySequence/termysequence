// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLineEdit;
class QPushButton;
class QLabel;
QT_END_NAMESPACE
class ConnectSettings;
class LaunchSettings;
class ServerInstance;

//
// Connection status
//
class ConnectStatusDialog final: public QDialog
{
    Q_OBJECT

private:
    QPlainTextEdit *m_output;
    QLineEdit *m_input;
    QPushButton *m_inputButton;
    QPushButton *m_restartButton;
    QLabel *m_status, *m_icon;

    ConnectSettings *m_conninfo;

    void setIcon(int standardIcon);

private slots:
    void handleInput();
    void handleRestart();

signals:
    void inputRequest(QString input);

public:
    ConnectStatusDialog(QWidget *parent, ConnectSettings *conninfo);
    ~ConnectStatusDialog();

    QSize sizeHint() const { return QSize(640, 400); }

    void appendOutput(const QString &output);
    void setFailure(const QString &status);

    void bringUp();
};

//
// Command status
//
class CommandStatusDialog final: public QWidget
{
    Q_OBJECT

private:
    QPlainTextEdit *m_output;
    QLabel *m_status;

    int m_pid;

private slots:
    void scrollRequest();

protected:
    bool event(QEvent *event);

public:
    CommandStatusDialog(LaunchSettings *launcher, ServerInstance *server);

    QSize sizeHint() const { return QSize(640, 400); }

    void appendOutput(const QString &output);
    void setPid(int pid);
    void setFinished();
    void setFailure(const QString &msg);

    void bringUp();
};
