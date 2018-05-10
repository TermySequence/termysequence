// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "window.h"
#include "iconmodel.h"
#include "workmodel.h"
#include "status.h"
#include "settings.h"
#include "dialog.h"
#include "setup.h"
#include "log.h"

#include <QStatusBar>
#include <QMenuBar>
#include <QStackedWidget>
#include <QCloseEvent>
#include <QInputDialog>

MainWindow *g_window;

//
// Window
//
MainWindow::MainWindow()
{
    setWindowTitle(ICONTOOL_NAME " " PROJECT_VERSION);
    setAttribute(Qt::WA_DeleteOnClose, true);

    m_log = new LogWindow;

    restoreGeometry(g_settings->windowGeometry());

    statusBar()->addPermanentWidget(g_status->getWidget());
    statusBar()->setVisible(true);

    m_stack = new QStackedWidget;
    m_stack->addWidget(g_iconview = new IconView);
    m_stack->addWidget(g_workview = new WorkView);
    setCentralWidget(m_stack);

    createMenus();
}

void
MainWindow::bringUp()
{
    if (g_status->loadIssue())
        m_log->bringUp();

    show();
}

void
MainWindow::closeEvent(QCloseEvent *event)
{
    g_settings->setWindowGeometry(saveGeometry());
    g_settings->sync();
    event->accept();
}

void
MainWindow::popSetup()
{
    (new SetupDialog(this))->show();
}

void
MainWindow::popOptions()
{
    (new SettingsDialog(this))->show();
}

void
MainWindow::popSearch()
{
    bool ok;
    QString str = m_stack->currentIndex() == 0 ?
        g_iconmodel->search() :
        g_workfilter->search();

    str = QInputDialog::getText(this, A("Search"), A("Search string:"),
                                QLineEdit::Normal, str, &ok);
    if (ok) {
        if (m_stack->currentIndex() == 0)
            g_iconmodel->setSearch(str);
        else
            g_workfilter->setSearch(str);
    }
}

void
MainWindow::resetSearch()
{
    if (m_stack->currentIndex() == 0)
        g_iconmodel->setSearch(g_mtstr);
    else
        g_workfilter->setSearch(g_mtstr);
}

void
MainWindow::createMenus()
{
    QMenuBar *bar = menuBar();
    QMenu *menu;
    QAction *a;
    QActionGroup *group = new QActionGroup(this);

    bar->addMenu(menu = new QMenu(A("File"), bar));
    menu->addAction(A("Save"), g_workmodel, &WorkModel::save);
    menu->addAction(A("Setup..."), this, &MainWindow::popSetup);
    menu->addSeparator();
    menu->addAction(A("Update..."), g_workmodel, &WorkModel::makeUpdate, Qt::Key_F9);
    menu->addAction(A("Install..."), g_workmodel, &WorkModel::makeInstall, Qt::Key_F10);
    menu->addSeparator();
    menu->addAction(A("Quit"), this, &QMainWindow::close);

    bar->addMenu(menu = new QMenu(A("Edit"), bar));
    menu->addAction(A("Search"), this, &MainWindow::popSearch, Qt::Key_F3);
    menu->addAction(A("Reset Search"), this, &MainWindow::resetSearch, Qt::Key_F4);
    menu->addSeparator();
    menu->addAction(A("Set Icon"), g_workview, &WorkView::setIcon);
    menu->addAction(A("Unset Icon"), g_workview, &WorkView::unsetIcon);
    menu->addSeparator();
    menu->addAction(A("Hide Selected Icons"), g_iconview, &IconView::hideIcons);

    bar->addMenu(menu = new QMenu(A("View"), bar));
    a = menu->addAction(A("Icons"), this, [this]{ m_stack->setCurrentIndex(0); }, Qt::Key_F1);
    a->setCheckable(true);
    a->setChecked(true);
    a->setActionGroup(group);
    a = menu->addAction(A("Work"), this, [this]{ m_stack->setCurrentIndex(1); }, Qt::Key_F2);
    a->setCheckable(true);
    a->setActionGroup(group);
    menu->addSeparator();
    menu->addAction(A("Event Log"), m_log, &LogWindow::bringUp, Qt::Key_F5);
    menu->addAction(A("Options..."), this, &MainWindow::popOptions, Qt::Key_F6);
}

//
// Display functions
//
QString
getInput(const QString &message, const QString &defval, bool *ok)
{
    if (g_window) {
        return QInputDialog::getText(g_window, A("Input"), message,
                                     QLineEdit::Normal, defval, ok);
    } else {
        *ok = true;
        return defval;
    }
}
