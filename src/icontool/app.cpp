// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app.h"
#include "iconmodel.h"
#include "workmodel.h"
#include "window.h"
#include "status.h"

#include <QCoreApplication>

IconApplication::IconApplication(QCoreApplication *app, int command) :
    QObject(app),
    m_app(app),
    m_command(command)
{
}

void
IconApplication::handleIconsLoaded()
{
    switch (m_command) {
    default:
        g_window = new MainWindow;
        g_window->bringUp();
        break;
    case 1:
        connect(g_workmodel, SIGNAL(finished()), m_app, SLOT(quit()));
        g_workmodel->makeUpdate();
        break;
    case 2:
        connect(g_workmodel, SIGNAL(finished()), m_app, SLOT(quit()));
        g_workmodel->makeInstall();
        break;
    }
}

int
IconApplication::exec()
{
    g_status = new MainStatus(m_command == 0);
    g_iconmodel = new IconModel(10, m_app);
    g_workmodel = new WorkModel(m_app);
    g_workfilter = new WorkFilter(m_app);

    connect(g_iconmodel, SIGNAL(loaded()), SLOT(handleIconsLoaded()));
    g_iconmodel->load(m_command == 0);
    return m_app->exec();
}
