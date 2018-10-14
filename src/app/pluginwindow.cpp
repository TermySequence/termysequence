// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "pluginwindow.h"
#include "attrbase.h"
#include "iconbutton.h"
#include "messagebox.h"
#include "pluginmodel.h"
#include "plugin.h"
#include "settings/settings.h"

#include <QHBoxLayout>
#include <QKeyEvent>

#define TR_BUTTON1 TL("input-button", "Unload Plugin")
#define TR_BUTTON2 TL("input-button", "Reload Plugin")
#define TR_BUTTON3 TL("input-button", "Reload All")
#define TR_BUTTON4 TL("input-button", "Expand All")
#define TR_BUTTON5 TL("input-button", "Collapse All")
#define TR_TITLE1 TL("window-title", "Manage Plugins")

PluginsWindow *g_pluginwin;

PluginsWindow::PluginsWindow()
{
    setWindowTitle(TR_TITLE1);
    setAttribute(Qt::WA_QuitOnClose, false);
    setSizeGripEnabled(true);

    m_view = new PluginView;

    m_unloadButton = new IconButton(ICON_REMOVE_ITEM, TR_BUTTON1);
    m_reloadButton = new IconButton(ICON_RELOAD, TR_BUTTON2);
    auto *refreshButton = new IconButton(ICON_RELOAD_ALL, TR_BUTTON3);
    auto *expandButton = new QPushButton(TR_BUTTON4);
    auto *collapseButton = new QPushButton(TR_BUTTON5);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Vertical);
    buttonBox->addButton(m_reloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(m_unloadButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(refreshButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(expandButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(collapseButton, QDialogButtonBox::ActionRole);
    buttonBox->addHelpButton("manage-plugins");

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(m_view);
    layout->addWidget(buttonBox);
    setLayout(layout);

    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));

    if (!i) {
        m_unloadButton->setEnabled(false);
        m_reloadButton->setEnabled(false);
        refreshButton->setEnabled(false);
        return;
    }

    connect(g_settings, SIGNAL(pluginsReloaded()), SLOT(handleReset()));
    connect(m_view->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(handleSelection()));

    connect(m_unloadButton, SIGNAL(clicked()), SLOT(handleUnloadPlugin()));
    connect(m_reloadButton, SIGNAL(clicked()), SLOT(handleReloadPlugin()));
    connect(refreshButton, SIGNAL(clicked()), SLOT(handleReload()));
    connect(expandButton, SIGNAL(clicked()), SLOT(handleExpand()));
    connect(collapseButton, SIGNAL(clicked()), SLOT(handleCollapse()));

    handleSelection();
}

bool
PluginsWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(event)->matches(QKeySequence::Cancel))
            close();
        break;
    case QEvent::WindowActivate:
        m_view->setFocus(Qt::ActiveWindowFocusReason);
        break;
    default:
        break;
    }

    return QDialog::event(event);
}

void
PluginsWindow::bringUp()
{
    if (m_expanding)
        m_view->expandAll();

    show();
    raise();
    activateWindow();
}

void
PluginsWindow::handleReset()
{
    // this happens after model has received the signal
    if (m_expanding)
        m_view->expandAll();

    handleSelection();
}

void
PluginsWindow::handleSelection()
{
    bool empty = m_view->selectionModel()->selectedIndexes().isEmpty();

    m_unloadButton->setEnabled(!empty);
    m_reloadButton->setEnabled(!empty);
}

void
PluginsWindow::handleUnloadPlugin()
{
    auto *plugin = m_view->selectedPlugin();
    if (plugin)
        g_settings->unloadPlugin(plugin);
}

void
PluginsWindow::handleReloadPlugin()
{
    auto *plugin = m_view->selectedPlugin();
    if (plugin)
        g_settings->reloadPlugin(plugin);
}

void
PluginsWindow::handleReload()
{
    Plugin::reloading();
    g_settings->rescanPlugins();
}

void
PluginsWindow::handleExpand()
{
    m_expanding = true;
    m_view->expandAll();
}

void
PluginsWindow::handleCollapse()
{
    m_expanding = false;
    m_view->collapseAll();
}
