// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app.h"
#include "datastore.h"
#include "logwindow.h"
#include "plugin.h"
#include "base/mainwindow.h"
#include "base/listener.h"
#include "base/manager.h"
#include "base/thumbicon.h"
#include "base/termwidget.h"
#include "base/dragicon.h"
#include "settings/settings.h"
#include "settings/state.h"
#include "settings/setupdialog.h"

#include <QApplication>
#include <QSocketNotifier>
#include <QMessageBox>

#define TR_TEXT1 TL("window-text", "Waiting for server handshake to complete") + A("...")

TermApplication::TermApplication(QApplication *app) :
    QObject(app),
    m_app(app)
{
    g_settings = new TermSettings;

    ThumbIconCache::initialize();

    g_logwin = new LogWindow;
    g_datastore = new DatastoreController;

    Plugin::initialize();
    TermManager::initialize();
    ThumbIcon::initialize();
    TermWidget::initialize();
    DragIcon::initialize();

    g_settings->loadFolders();
}

void
TermApplication::start(int sigfd)
{
    m_notifier = new QSocketNotifier(sigfd, QSocketNotifier::Read, this);
    connect(m_notifier, SIGNAL(activated(int)), SLOT(handleSignal()));

    if (g_state->suppressSetup()) {
        begin();
    } else {
        auto *dialog = new SetupDialog;
        connect(dialog, &QDialog::accepted, this, [this]{
            g_state->setSuppressSetup(true);
            begin();
        });
        connect(dialog, SIGNAL(rejected()), m_app, SLOT(quit()));
        dialog->show();
    }
}

void
TermApplication::begin()
{
    g_listener = new TermListener;
    m_mocReady = connect(g_listener, SIGNAL(ready()), SLOT(handleReady()));

    m_timerId = startTimer(1000);
    g_listener->start();
}

void
TermApplication::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timerId) {
        m_box = new QMessageBox(QMessageBox::NoIcon, FRIENDLY_NAME, TR_TEXT1,
                                QMessageBox::Cancel);

        connect(m_box, SIGNAL(finished(int)), SLOT(handleCancel()));
        m_box->show();

        killTimer(m_timerId);
        m_timerId = 0;
    }
}

void
TermApplication::handleSignal()
{
    m_notifier->setEnabled(false);
    if (g_listener) {
        g_listener->disconnect(this);
        g_listener->quit();
    }
    m_app->exit(EXITCODE_SIGNAL);
}

void
TermApplication::handleCancel()
{
    m_notifier->setEnabled(false);
    disconnect(m_mocReady);
    m_app->exit(EXITCODE_CANCEL);
}

void
TermApplication::handleReady()
{
    disconnect(m_mocReady);

    if (m_timerId) {
        killTimer(m_timerId);
        m_timerId = 0;
    }
    if (m_box) {
        m_box->disconnect(this);
        delete m_box;
        m_box = nullptr;
    }

    auto manager = g_listener->createInitialManager();
    auto win = new MainWindow(manager);
    win->bringUp();

    if (g_state->showTotd()) {
        manager->actionTipOfTheDay();
    }
}

void
TermApplication::cleanup()
{
    if (m_timerId)
        killTimer(m_timerId);

    delete m_box;
    delete g_listener;
    delete g_datastore;
    delete g_logwin;
    delete g_settings;

    ThumbIcon::teardown();
    Plugin::teardown();
}
