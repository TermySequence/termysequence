// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE
class ServerInstance;
class TermInstance;
class InfoTab;
class ContentView;

class InfoWindow final: public QDialog
{
    Q_OBJECT

private:
    ServerInstance *m_server;
    TermInstance *m_term;

    QTabWidget *m_tabs;
    InfoTab *m_serverAttrTab;
    QWidget *m_serverPropTab;
    InfoTab *m_termAttrTab;
    QWidget *m_termPropTab;
    ContentView *m_contentTab;

private slots:
    void handleServerChange();
    void handleTermChange();

public:
    InfoWindow(ServerInstance *server, TermInstance *term = 0);

    QSize sizeHint() const { return QSize(880, 880); }

    void bringUp();

    static void showServerWindow(ServerInstance *server);
    static void showTermWindow(TermInstance *term);
    static void showContentWindow(TermInstance *term, const QString &id);
};
