// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "infowindow.h"
#include "infotab.h"
#include "infopropmodel.h"
#include "contentmodel.h"
#include "server.h"
#include "term.h"
#include "listener.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTabWidget>

#define TR_TAB1 TL("tab-title", "Inline Content")
#define TR_TAB2 TL("tab-title", "Terminal Properties")
#define TR_TAB3 TL("tab-title", "Terminal Attributes")
#define TR_TAB4 TL("tab-title", "Server Properties")
#define TR_TAB5 TL("tab-title", "Server Attributes")

static int s_lastTermTab = 1;
static int s_lastServerTab;

InfoWindow::InfoWindow(ServerInstance *server, TermInstance *term) :
    m_server(server),
    m_term(term)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setSizeGripEnabled(true);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    m_tabs = new QTabWidget;

    connect(server, SIGNAL(destroyed()), SLOT(deleteLater()));

    if (term) {
        connect(term, SIGNAL(destroyed()), SLOT(deleteLater()));

        auto *contentModel = new ContentModel(term->content(), this);
        m_contentTab = new ContentView(contentModel, term);
        m_tabs->addTab(m_contentTab, TR_TAB1);

        auto *termModel = new TermPropModel(term, this);
        m_termPropTab = new InfoPropView(termModel);
        m_tabs->addTab(m_termPropTab, TR_TAB2);

        m_termAttrTab = new InfoTab(term);
        m_tabs->addTab(m_termAttrTab, TR_TAB3);
        connect(m_termAttrTab, SIGNAL(changeSubmitted()), SLOT(handleTermChange()));
        connect(m_tabs, &QTabWidget::currentChanged, [](int i){ s_lastTermTab = i; });
    } else {
        connect(m_tabs, &QTabWidget::currentChanged, [](int i){ s_lastServerTab = i; });
    }

    auto *serverModel = new ServerPropModel(server, this, !term);
    m_serverPropTab = new InfoPropView(serverModel);
    m_tabs->addTab(m_serverPropTab, TR_TAB4);

    m_serverAttrTab = new InfoTab(server);
    m_tabs->addTab(m_serverAttrTab, TR_TAB5);
    connect(m_serverAttrTab, SIGNAL(changeSubmitted()), SLOT(handleServerChange()));

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_tabs, 1);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
}

void
InfoWindow::bringUp()
{
    if (m_term)
        m_contentTab->resizeColumnsToContents();

    show();
    raise();
    activateWindow();
}

void
InfoWindow::showServerWindow(ServerInstance *server)
{
    auto *win = server->infoWindow();

    if (!win)
        server->setInfoWindow(win = new InfoWindow(server));

    win->m_tabs->setCurrentIndex(s_lastServerTab);
    win->bringUp();
}

void
InfoWindow::showTermWindow(TermInstance *term)
{
    auto *win = term->infoWindow();

    if (!win)
        term->setInfoWindow(win = new InfoWindow(term->server(), term));

    win->m_tabs->setCurrentIndex(s_lastTermTab);
    win->bringUp();
}

void
InfoWindow::showContentWindow(TermInstance *term, const QString &id)
{
    auto *win = term->infoWindow();

    if (!win)
        term->setInfoWindow(win = new InfoWindow(term->server(), term));

    win->m_contentTab->selectId(id);
    win->m_tabs->setCurrentWidget(win->m_contentTab);
    win->bringUp();
}

void
InfoWindow::handleServerChange()
{
    if (m_serverAttrTab->removal()) {
        g_listener->pushServerAttributeRemove(m_server, m_serverAttrTab->key());
    } else {
        g_listener->pushServerAttribute(m_server, m_serverAttrTab->key(),
                                        m_serverAttrTab->value());
    }
}

void
InfoWindow::handleTermChange()
{
    if (m_termAttrTab->removal()) {
        g_listener->pushTermAttributeRemove(m_term, m_termAttrTab->key());
    } else {
        g_listener->pushTermAttribute(m_term, m_termAttrTab->key(),
                                      m_termAttrTab->value());
    }
}
