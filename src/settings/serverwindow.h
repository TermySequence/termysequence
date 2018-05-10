// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class ServerView;

class ServersWindow final: public QWidget
{
    Q_OBJECT

private:
    ServerView *m_view;

    QPushButton *m_editButton;
    QPushButton *m_deleteButton;
    QPushButton *m_reloadButton;

private slots:
    void handleSelection();

    void handleEditServer();
    void handleDeleteServer();
    void handleReload();

protected:
    bool event(QEvent *event);

public:
    ServersWindow();

    QSize sizeHint() const { return QSize(1024, 512); }

    void bringUp();
};

extern ServersWindow *g_serverwin;
