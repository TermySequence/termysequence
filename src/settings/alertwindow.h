// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class AlertView;

class AlertsWindow final: public QWidget
{
    Q_OBJECT

private:
    AlertView *m_view;

    QPushButton *m_newButton;
    QPushButton *m_cloneButton;
    QPushButton *m_deleteButton;
    QPushButton *m_renameButton;
    QPushButton *m_editButton;
    QPushButton *m_reloadButton;

private slots:
    void handleSelection();

    void handleNewAlert();
    void handleCloneAlert();
    void handleDeleteAlert();
    void handleRenameAlert();
    void handleEditAlert();
    void handleReload();

protected:
    bool event(QEvent *event);

public:
    AlertsWindow();

    QSize sizeHint() const { return QSize(1024, 512); }

    void bringUp();
};

extern AlertsWindow *g_alertwin;
