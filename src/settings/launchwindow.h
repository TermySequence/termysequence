// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class LauncherView;

class LaunchersWindow final: public QWidget
{
    Q_OBJECT

private:
    LauncherView *m_view;

    QPushButton *m_newButton;
    QPushButton *m_cloneButton;
    QPushButton *m_deleteButton;
    QPushButton *m_renameButton;
    QPushButton *m_editButton;
    QPushButton *m_importButton;
    QPushButton *m_reloadButton;

    void doImport(const QString &path);

private slots:
    void handleSelection();

    void handleNewLauncher();
    void handleCloneLauncher();
    void handleDeleteLauncher();
    void handleRenameLauncher();
    void handleEditLauncher();
    void handleImport();
    void handleReload();

protected:
    bool event(QEvent *event);

public:
    LaunchersWindow();

    QSize sizeHint() const { return QSize(1024, 512); }

    void bringUp();
};

extern LaunchersWindow *g_launchwin;
