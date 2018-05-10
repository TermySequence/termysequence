// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class ProfileView;

class ProfilesWindow final: public QWidget
{
    Q_OBJECT

private:
    ProfileView *m_view;

    QPushButton *m_newButton;
    QPushButton *m_cloneButton;
    QPushButton *m_deleteButton;
    QPushButton *m_renameButton;
    QPushButton *m_editButton;
    QPushButton *m_reloadButton;

private slots:
    void handleSelection();

    void handleNewProfile();
    void handleCloneProfile();
    void handleDeleteProfile();
    void handleRenameProfile();
    void handleEditProfile();
    void handleReload();

protected:
    bool event(QEvent *event);

public:
    ProfilesWindow();

    QSize sizeHint() const { return QSize(800, 480); }

    void bringUp();
};

extern ProfilesWindow *g_profilewin;
