// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class KeymapView;

class KeymapsWindow final: public QWidget
{
    Q_OBJECT

private:
    KeymapView *m_view;

    QPushButton *m_newButton;
    QPushButton *m_cloneButton;
    QPushButton *m_deleteButton;
    QPushButton *m_renameButton;
    QPushButton *m_editButton;
    QPushButton *m_reloadButton;

private slots:
    void handleSelection();

    void handleNewKeymap();
    void handleCloneKeymap();
    void handleDeleteKeymap();
    void handleRenameKeymap();
    void handleEditKeymap();
    void handleReload();

protected:
    bool event(QEvent *event);

public:
    KeymapsWindow();

    QSize sizeHint() const { return QSize(800, 480); }

    void bringUp();
};

extern KeymapsWindow *g_keymapwin;
