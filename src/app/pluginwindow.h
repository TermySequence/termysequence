// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class PluginView;

class PluginsWindow final: public QDialog
{
    Q_OBJECT

private:
    PluginView *m_view;

    QPushButton *m_unloadButton;
    QPushButton *m_reloadButton;
    QPushButton *m_refreshButton;

private slots:
    void handleReset();
    void handleSelection();

    void handleUnloadPlugin();
    void handleReloadPlugin();
    void handleReload();

protected:
    bool event(QEvent *event);

public:
    PluginsWindow();

    QSize sizeHint() const { return QSize(800, 480); }

    void bringUp();
};

extern PluginsWindow *g_pluginwin;
