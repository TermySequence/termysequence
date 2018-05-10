// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE
class LogWindow;

//
// Window
//
class MainWindow final: public QMainWindow
{
    Q_OBJECT

private:
    QStackedWidget *m_stack;
    LogWindow *m_log;

    void createMenus();

protected:
    void closeEvent(QCloseEvent *event);

public:
    MainWindow();

    void bringUp();

    void popSetup();
    void popOptions();
    void popSearch();
    void resetSearch();

    QSize sizeHint() const { return QSize(1024, 768); }
};

extern MainWindow *g_window;

//
// Display functions
//
extern QString
getInput(const QString &message, const QString &defval, bool *ok);
