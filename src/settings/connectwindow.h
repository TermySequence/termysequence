// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class ConnectView;

class ConnectsWindow final: public QWidget
{
    Q_OBJECT

private:
    ConnectView *m_view;

    QPushButton *m_newButton;
    QPushButton *m_cloneButton;
    QPushButton *m_deleteButton;
    QPushButton *m_renameButton;
    QPushButton *m_editButton;
    QPushButton *m_launchButton;
    QPushButton *m_stopButton;
    QPushButton *m_reloadButton;

private slots:
    void handleSelection();

    void handleNewConn();
    void handleCloneConn();
    void handleDeleteConn();
    void handleRenameConn();
    void handleEditConn();
    void handleLaunchConn();
    void handleReload();

protected:
    bool event(QEvent *event);

public:
    ConnectsWindow();

    QSize sizeHint() const { return QSize(1024, 512); }

    void bringUp();
};

extern ConnectsWindow *g_connwin;
