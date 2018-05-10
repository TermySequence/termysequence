// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class PortView;
class PortEditor;

class PortsWindow final: public QWidget
{
    Q_OBJECT

private:
    PortView *m_view;
    PortEditor *m_dialog = nullptr;

    QPushButton *m_newButton;
    QPushButton *m_editButton;
    QPushButton *m_startButton;
    QPushButton *m_cancelButton;
    QPushButton *m_killButton;

private slots:
    void handleSelection();

    void handleNewPort();
    void handleEditServer();
    void handleStartPort();
    void handleInspectPort();
    void handleCancelPort();
    void handleKillConn();
    void handleLaunch();

protected:
    bool event(QEvent *event);

public:
    PortsWindow();

    QSize sizeHint() const { return QSize(800, 480); }

    void bringUp();
};

extern PortsWindow *g_portwin;
