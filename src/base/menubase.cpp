// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/actions.h"
#include "app/attr.h"
#include "app/icons.h"
#include "app/messagebox.h"
#include "menubase.h"
#include "listener.h"
#include "mainwindow.h"
#include "manager.h"
#include "server.h"
#include "term.h"
#include "buffers.h"
#include "scrollport.h"
#include "selection.h"
#include "dockwidget.h"
#include "jobwidget.h"
#include "notewidget.h"
#include "filewidget.h"
#include "taskwidget.h"
#include "thumbicon.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "settings/profile.h"
#include "settings/keymap.h"
#include "settings/servinfo.h"
#include "settings/termlayout.h"

#define OBJPROP_SLOT            "dynSlot"
#define OBJPROP_PROFILE_NAME    "dynProfileName"
#define OBJPROP_CONFIRM         "dynConfirmMsg"

#define AFLAGS(a) a->property(OBJPROP_FLAG).toULongLong()

#define TR_TEXT1 TL("window-text", "%1 in keymap %2")
// #define TR_TEXT2 TL("window-text", "no shortcut in keymap %1")
#define TR_TITLE1 TL("window-title", "Confirm Action")

inline void
DynamicMenu::setup(const char *id, MainWindow *window)
{
    m_window = window;
    m_id = id;
    m_manager = window->manager();

    connect(this, SIGNAL(aboutToShow()), SLOT(preshow()));
    connect(this, SIGNAL(aboutToHide()), m_manager, SIGNAL(menuFinished()));
}

DynamicMenu::DynamicMenu(const char *id, MainWindow *window, QWidget *parent) :
    QMenu(parent),
    m_term(nullptr),
    m_server(nullptr)
{
    setup(id, window);
}

DynamicMenu::DynamicMenu(TermInstance *term, const char *id,
                         MainWindow *window, QWidget *parent) :
    QMenu(parent),
    m_term(term),
    m_server(nullptr)
{
    setup(id, window);

    if (term) {
        connect(term, SIGNAL(destroyed()), SLOT(deleteLater()));
    } else {
        connect(m_manager, SIGNAL(termActivated(TermInstance*,TermScrollport*)), SLOT(handleTermActivated(TermInstance*)));
        m_term = m_manager->activeTerm();
    }
}

DynamicMenu::DynamicMenu(ServerInstance *server, const char *id,
                         MainWindow *window, QWidget *parent) :
    QMenu(parent),
    m_term(nullptr),
    m_server(server)
{
    setup(id, window);

    if (server) {
        connect(server, SIGNAL(destroyed()), SLOT(deleteLater()));
    } else {
        connect(m_manager, SIGNAL(serverActivated(ServerInstance*)), SLOT(handleServerActivated(ServerInstance*)));
        m_server = m_manager->activeServer();
    }
}

void
DynamicMenu::handleTermActivated(TermInstance *term)
{
    if (m_term != term) {
        m_term = term;
    }
}

void
DynamicMenu::handleServerActivated(ServerInstance *server)
{
    if (m_server != server) {
        m_server = server;

        if (m_feature == NewTermMenu)
            updateNewTerm();
        else if (m_feature == NewWindowMenu)
            updateNewWindow();
    }
}

void
DynamicMenu::setConfirmation(QAction *a, const QString &message)
{
    a->setProperty(OBJPROP_CONFIRM, message);
    disconnect(a, SIGNAL(triggered()), this, SLOT(handleAction()));
    connect(a, SIGNAL(triggered()), SLOT(handleConfirmation()));
}

void
DynamicMenu::handleAction()
{
    m_window->menuAction(sender()->property(OBJPROP_SLOT).toString());
}

void
DynamicMenu::handleConfirmation()
{
    auto *a = sender();
    const QString slot = a->property(OBJPROP_SLOT).toString();
    const QString msg = a->property(OBJPROP_CONFIRM).toString();

    // no this pointer in the lambda
    MainWindow *window = m_window;

    auto *box = askBox(TR_TITLE1, msg, window);
    connect(box, &QDialog::finished, [window, slot](int result) {
        if (result == QMessageBox::Yes) {
            window->menuAction(slot);
        }
    });
    box->show();
}

void
DynamicMenu::enable(MenuFeature feature)
{
    switch (m_feature = feature) {
    case ServerTermMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowServerTermMenu()));
        // we have to run before preshow
        disconnect(this, SIGNAL(aboutToShow()), this, SLOT(preshow()));
        connect(this, SIGNAL(aboutToShow()), SLOT(preshow()));
        connect(g_global, SIGNAL(menuSizeChanged()), SLOT(updateServerTermMenu()));
        updateServerTermMenu();
        break;
    case LocalTermMenu:
        connect(g_settings, SIGNAL(profilesChanged()), SLOT(updateLocalTerm()));
        updateLocalTerm();
        break;
    case NewTermMenu:
        connect(g_settings, SIGNAL(profilesChanged()), SLOT(updateNewTerm()));
        updateNewTerm();
        break;
    case NewWindowMenu:
        connect(g_settings, SIGNAL(profilesChanged()), SLOT(updateNewWindow()));
        updateNewWindow();
        break;
    case NewConnMenu:
        connect(g_settings, SIGNAL(connectionsChanged()), SLOT(updateNewConn()));
        updateNewConn();
        break;
    case SettingsMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowEditSettings()));
        break;
    case TermMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowTermMenu()));
        break;
    case ConnectMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowConnectMenu()));
        connect(g_settings, SIGNAL(connectionsChanged()), SLOT(updateConnectMenu()));
        break;
    case LayoutMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowLayoutMenu()));
        break;
    case EditMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowEditMenu()));
        break;
    case InputMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowInputMenu()));
        break;
    case ServerMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowServerMenu()));
        break;
    case ToolsMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowToolsMenu()));
        connect(g_settings, SIGNAL(launchersChanged()), SLOT(updateToolsMenu()));
        break;
    case OpenFileMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowOpenFileMenu()));
        goto setupLaunchMenu;
    case OpenTaskMenu:
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowOpenTaskMenu()));
    setupLaunchMenu:
        // we have to run before preshow
        disconnect(this, SIGNAL(aboutToShow()), this, SLOT(preshow()));
        connect(this, SIGNAL(aboutToShow()), SLOT(preshow()));
        connect(g_global, SIGNAL(menuSizeChanged()), SLOT(updateLaunchMenu()));
        updateLaunchMenu();
        break;
    case AlertMenu:
        connect(g_settings, SIGNAL(alertsChanged()), SLOT(updateAlerts()));
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowAlerts()));
        updateAlerts();
        break;
    case SwitchProfileMenu:
        connect(g_settings, SIGNAL(profilesChanged()), SLOT(updateSwitchProfile()));
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowSwitchProfile()));
        updateSwitchProfile();
        break;
    case PushProfileMenu:
        connect(g_settings, SIGNAL(profilesChanged()), SLOT(updatePushProfile()));
        connect(this, SIGNAL(aboutToShow()), SLOT(preshowSwitchProfile()));
        updatePushProfile();
        break;
    case TermIconMenu:
        connect(g_settings, SIGNAL(iconsChanged()), SLOT(updateTermIcons()));
        updateTermIcons();
        break;
    case ServerIconMenu:
        connect(g_settings, SIGNAL(iconsChanged()), SLOT(updateServerIcons()));
        updateServerIcons();
        break;
    }
}

void
DynamicMenu::enable(QAction *a, uint64_t flag)
{
    switch (flag) {
    case 0:
        a->setEnabled(false);
        // fallthru
    case 1:
        break;
    default:
        a->setDynFlag(flag);

        if (flag & DynAutoHide)
            a->setVisible(false);
        if (flag & DynCheckable)
            a->setCheckable(true);
    }
}

QAction *
DynamicMenu::addAction()
{
    return QMenu::addAction(g_mtstr, this, SLOT(handleAction()));
}

QAction *
DynamicMenu::addAction(const char *text, const char *tooltip,
                       const std::string &icon, const QString &slot,
                       uint64_t flag)
{
    QAction *a = QMenu::addAction(ThumbIcon::fromTheme(icon),
                                  QCoreApplication::translate(m_id, text),
                                  this, SLOT(handleAction()));

    a->setToolTip(QCoreApplication::translate("action", tooltip));
    a->setProperty(OBJPROP_SLOT, slot);
    enable(a, flag);
    return a;
}

QAction *
DynamicMenu::addAction(const QString &text, const QString &tooltip,
                       const std::string &icon, const QString &slot)
{
    QAction *a = QMenu::addAction(ThumbIcon::fromTheme(icon), text,
                                  this, SLOT(handleAction()));

    a->setToolTip(tooltip);
    a->setProperty(OBJPROP_SLOT, slot);
    return a;
}

QAction *
DynamicMenu::addAction(const QString &text, const QString &tooltip,
                       const QIcon &icon, const QString &slot,
                       uint64_t flag)
{
    QAction *a = QMenu::addAction(icon, text, this, SLOT(handleAction()));

    a->setToolTip(tooltip);
    a->setProperty(OBJPROP_SLOT, slot);
    enable(a, flag);
    return a;
}

inline void
DynamicMenu::adjustAction(QAction *action,
                          const QString &text, const QString &tooltip,
                          const QIcon &icon, const QString &slot)
{
    action->setIcon(icon);
    action->setText(text);
    action->setToolTip(tooltip);
    action->setProperty(OBJPROP_SLOT, slot);
}

QAction *
DynamicMenu::addMenu(QMenu *subMenu, const char *text)
{
    subMenu->setTitle(QCoreApplication::translate(m_id, text));
    return QMenu::addMenu(subMenu);
}

QAction *
DynamicMenu::addMenu(QMenu *subMenu, const char *text, const std::string &icon)
{
    subMenu->setTitle(QCoreApplication::translate(m_id, text));
    subMenu->setIcon(ThumbIcon::fromTheme(icon));
    return QMenu::addMenu(subMenu);
}

void
DynamicMenu::updateLocalTerm()
{
    QString slot(L("NewLocalTerminal|"));
    QString tip(ACT_NEW_LOCAL_TERMINAL_ARG);

    clear();

    for (auto &i: g_settings->primaryProfiles())
    {
        addAction(i.first, tip.arg(i.first), i.second, slot + i.first);
    }
    if (g_settings->haveSecondaryProfiles()) {
        addSeparator();
        addAction(TN("dmenu-newterm", "More") "...",
                  ACT_NEW_LOCAL_TERMINAL_CHOOSE, ICON_CHOOSE_ITEM,
                  slot + g_str_PROMPT_PROFILE);
    }
}

void
DynamicMenu::updateNewTerm()
{
    QString slot(L("NewTerminal|%1"));
    QString tip(ACT_NEW_TERMINAL_ARG);

    if (m_server)
        slot += '|' + m_server->idStr();

    clear();

    for (auto &i: g_settings->primaryProfiles())
    {
        addAction(i.first, tip.arg(i.first), i.second, slot.arg(i.first));
    }
    if (g_settings->haveSecondaryProfiles()) {
        addSeparator();
        addAction(TN("dmenu-newterm", "More") "...",
                  ACT_NEW_TERMINAL_CHOOSE, ICON_CHOOSE_ITEM,
                  slot.arg(g_str_PROMPT_PROFILE));
    }
}

void
DynamicMenu::updateNewWindow()
{
    QString slot(L("NewWindow|%1"));
    QString tip(ACT_NEW_WINDOW_ARG);

    if (m_server)
        slot += '|' + m_server->idStr();

    clear();

    for (auto &i: g_settings->primaryProfiles())
    {
        addAction(i.first, tip.arg(i.first), i.second, slot.arg(i.first));
    }
    if (g_settings->haveSecondaryProfiles()) {
        addSeparator();
        addAction(TN("dmenu-newwin", "More") "...",
                  ACT_NEW_WINDOW_CHOOSE, ICON_CHOOSE_ITEM,
                  slot.arg(g_str_PROMPT_PROFILE));
    }
}

void
DynamicMenu::updateNewConn()
{
    QString slot(L("OpenConnection|"));
    QString tip(ACT_OPEN_CONNECTION_ARG);

    clear();

    for (auto &i: g_settings->primaryConns())
    {
        addAction(i.first, tip.arg(i.first), i.second, slot + i.first);
    }
}

void
DynamicMenu::updateAlerts()
{
    QString slot1(L("SetAlert|%1")), slot2(L("ClearAlert"));
    QString tip(ACT_SET_ALERT_ARG);

    if (m_term) {
        slot1 += '|' + m_term->idStr();
        slot2 += '|' + m_term->idStr();
    }

    clear();

    addAction(TN("dmenu-alert", "Clear current alert"),
              ACT_CLEAR_ALERT, ICON_CLEAR_ALERT, slot2, DynClearAlert);
    addSeparator();

    for (auto &i: g_settings->primaryAlerts())
    {
        addAction(i.first, tip.arg(i.first), i.second, slot1.arg(i.first));
    }
    if (g_settings->haveSecondaryAlerts())
    {
        addSeparator();
        addAction(TN("dmenu-alert", "More") "...",
                  ACT_SET_ALERT, ICON_CHOOSE_ITEM,
                  slot1.arg(g_str_PROMPT_PROFILE));
    }
}

void
DynamicMenu::updateSwitchProfile()
{
    QString slot(L("SwitchProfile|%1"));
    QString tip(ACT_SWITCH_PROFILE_ARG);

    if (m_term)
        slot += '|' + m_term->idStr();

    clear();

    for (auto &i: g_settings->primaryProfiles())
    {
        addAction(i.first, tip.arg(i.first), i.second, slot.arg(i.first))
            ->setProperty(OBJPROP_PROFILE_NAME, i.first);
    }
    if (g_settings->haveSecondaryProfiles())
    {
        addSeparator();
        addAction(TN("dmenu-profile", "More") "...",
                  ACT_SWITCH_PROFILE, ICON_CHOOSE_ITEM,
                  slot.arg(g_str_PROMPT_PROFILE));
    }
}

void
DynamicMenu::updatePushProfile()
{
    QString slot(L("PushProfile|%1"));
    QString tip(ACT_PUSH_PROFILE_ARG);

    if (m_term)
        slot += '|' + m_term->idStr();

    clear();

    for (auto &i: g_settings->primaryProfiles())
    {
        addAction(i.first, tip.arg(i.first), i.second, slot.arg(i.first))
            ->setProperty(OBJPROP_PROFILE_NAME, i.first);
    }
    if (g_settings->haveSecondaryProfiles())
    {
        addSeparator();
        addAction(TN("dmenu-profile", "More") "...",
                  ACT_PUSH_PROFILE, ICON_CHOOSE_ITEM,
                  slot.arg(g_str_PROMPT_PROFILE));
    }
}

void
DynamicMenu::updateTermIcons()
{
    QString slot(L("SetTerminalIcon|%1"));
    QString tip(ACT_SET_TERMINAL_ICON_ARG);

    if (m_term)
        slot += '|' + m_term->idStr();

    clear();

    addAction(TN("dmenu-icon", "Clear custom icon"),
              ACT_SET_TERMINAL_ICON_DEFAULT, ICON_RESET_ICON,
              slot.arg(g_mtstr));
    addAction(TN("dmenu-icon", "No icon"),
              ACT_SET_TERMINAL_ICON_EMPTY, ICON_SET_NO_ICON,
              slot.arg(TI_NONE_NAME));
    addSeparator();

    for (auto &i: ThumbIcon::getFavoriteIcons(ThumbIcon::TerminalType))
    {
        addAction(i.first, tip.arg(i.first), i.second, slot.arg(i.first));
    }

    addSeparator();
    addAction(TN("dmenu-icon", "Choose") "...",
              ACT_SET_TERMINAL_ICON, ICON_CHOOSE_ITEM,
              slot.arg(g_str_PROMPT_PROFILE));
}

void
DynamicMenu::updateServerIcons()
{
    QString slot(L("SetServerIcon|%1"));
    QString tip(ACT_SET_SERVER_ICON_ARG);

    if (m_server)
        slot += '|' + m_server->idStr();

    clear();

    addAction(TN("dmenu-icon", "Clear custom icon"),
              ACT_SET_SERVER_ICON_DEFAULT, ICON_RESET_ICON,
              slot.arg(g_mtstr));
    addSeparator();

    for (auto &i: ThumbIcon::getFavoriteIcons(ThumbIcon::ServerType))
    {
        addAction(i.first, tip.arg(i.first), i.second, slot.arg(i.first));
    }

    addSeparator();
    addAction(TN("dmenu-icon", "Choose") "...",
              ACT_SET_SERVER_ICON, ICON_CHOOSE_ITEM,
              slot.arg(g_str_PROMPT_PROFILE));
}

void
DynamicMenu::preshowEditSettings()
{
    const auto *profile = m_term ? m_term->profile() : g_settings->defaultProfile();
    const auto *keymap = m_term ? m_term->keymap() : g_settings->defaultKeymap();
    QString profileText(QCoreApplication::translate(m_id, "&Edit Profile \"%1\""));
    QString keymapText(QCoreApplication::translate(m_id, "Edit &Keymap \"%1\""));

    for (auto i: actions())
        switch (AFLAGS(i)) {
        case DynEditProfile:
            i->setText(profileText.arg(profile->name()));
            break;
        case DynEditKeymap:
            i->setText(keymapText.arg(keymap->name()));
            break;
        }
}

void
DynamicMenu::preshowAlerts()
{
    bool isalert = m_term && m_term->alert();

    for (auto i: actions())
        if (AFLAGS(i) == DynClearAlert) {
            i->setVisible(isalert);
            break;
        }
}

void
DynamicMenu::preshowSwitchProfile()
{
    const auto *profile = m_term ? m_term->profile() : g_settings->defaultProfile();

    for (auto i: actions()) {
        bool b = i->property(OBJPROP_PROFILE_NAME).toString() != profile->name();
        i->setEnabled(b);
    }
}

void
DynamicMenu::preshowTermMenu()
{
    TermScrollport *scrollport = m_manager->activeScrollport();

    for (auto i: actions())
        switch (AFLAGS(i)) {
        case DynNotOurTerm:
            i->setEnabled(m_term && !m_term->ours());
            break;
        case DynRestoreMenuBar:
            i->setVisible(!m_window->menuBarVisible());
            break;
        case DynExitFullScreenMode:
            i->setVisible(m_window->isFullScreen());
            break;
        case DynExitPresentationMode:
            i->setVisible(g_listener->presMode());
            break;
        case DynFollowable:
            i->setChecked(scrollport && scrollport->followable());
            i->setEnabled(m_term && !m_term->ours());
            break;
        case DynInputtable:
            i->setChecked(m_term && m_term->remoteInput());
            i->setEnabled(m_term && m_term->ours());
            break;
        case DynHasPeer:
            i->setVisible(m_term && m_term->peer());
            break;
        case DynInputFollower:
            i->setChecked(m_term && m_term->inputFollower());
            i->setVisible(m_term && g_listener->inputLeader() &&
                          m_term != g_listener->inputLeader());
            break;
        case DynInputMultiplexing:
            i->setVisible(m_term && m_term == g_listener->inputLeader());
            break;
        }
}

void
DynamicMenu::updateConnectMenu()
{
    QString slot(L("OpenConnection|"));
    QString tip(ACT_OPEN_CONNECTION_ARG);

    auto conns = g_settings->menuConns();
    int idx = 0;

    for (auto i: actions())
        if (AFLAGS(i) == DynConnItem) {
            const auto &elt = conns[idx];
            if (!elt.first.isEmpty()) {
                adjustAction(i, elt.first, tip.arg(elt.first), elt.second, slot + elt.first);
                i->setVisible(true);
            } else {
                i->setVisible(false);
            }
            ++idx;
        }
}

void
DynamicMenu::preshowConnectMenu()
{
    for (auto i: actions())
        switch (AFLAGS(i)) {
        case DynTransientConn:
            i->setEnabled(!g_listener->transientServer());
            break;
        case DynPersistentConn:
            i->setEnabled(!g_listener->persistentServer());
            break;
        }
}

void
DynamicMenu::preshowEditMenu()
{
    TermInstance *term = m_manager->activeTerm();
    bool haveSelection = term ? !term->buffers()->selection()->isEmpty() : false;

    for (auto i: actions())
        if (AFLAGS(i) == DynHaveSelection)
            i->setEnabled(haveSelection);
}

void
DynamicMenu::preshowInputMenu()
{
    auto *term = m_term ? m_term : m_manager->activeTerm();

    for (auto i: actions())
        switch (AFLAGS(i)) {
        case DynInputLeader:
            i->setEnabled(term && term != g_listener->inputLeader());
            break;
        case DynInputFollower:
            i->setChecked(term && term->inputFollower());
            i->setEnabled(term && g_listener->inputLeader() &&
                          term != g_listener->inputLeader());
            break;
        case DynInputMultiplexing:
            i->setEnabled(g_listener->inputLeader());
            break;
        }
}

void
DynamicMenu::preshowLayoutMenu()
{
    TermInstance *term = m_manager->activeTerm();
    TermLayout layout;
    if (term)
        layout.parseLayout(term->layout());

    for (auto i: actions()) {
        int idx;

        switch (AFLAGS(i)) {
        case DynLayoutItem1:
            idx = 1;
            break;
        case DynLayoutItem2:
            idx = 2;
            break;
        case DynLayoutItem3:
            idx = 3;
            break;
        case DynLayoutItem4:
            idx = 4;
            break;
        default:
            continue;
        }

        i->setEnabled(term);
        i->setChecked(layout.itemEnabled(idx));
    }
}

void
DynamicMenu::preshowServerMenu()
{
    QString newText(QCoreApplication::translate(m_id, "&New Terminal on %1"));
    QString editText(QCoreApplication::translate(m_id, "&Edit Server \"%1\""));

    for (auto i: actions())
        switch (AFLAGS(i)) {
        case DynNewTerminal:
            i->setVisible(m_server);
            if (m_server)
                i->setText(newText.arg(m_server->shortname()));
            break;
        case DynEditServer:
            i->setVisible(m_server);
            if (m_server)
                i->setText(editText.arg(m_server->shortname()));
            return;
        }
}

void
DynamicMenu::updateToolsMenu()
{
    for (auto i: actions())
        switch (AFLAGS(i)) {
        case DynFilesToolOpenFile:
        case DynTasksToolOpenFile:
            const auto elt = g_settings->defaultLauncher();
            i->setIcon(elt.second);
            i->setText(QCoreApplication::translate(m_id, "Open with %1").arg(elt.first));
            break;
        }
}

void
DynamicMenu::preshowToolsMenu()
{
    const auto *lastTool = m_window->lastTool();
    uint64_t toolFlag = lastTool->toolFlag();

    for (auto i: actions()) {
        uint64_t flags = AFLAGS(i);
        uint64_t needFlags = flags & DynToolMask;
        if (needFlags && needFlags != toolFlag) {
            i->setVisible(false);
            continue;
        }

        bool visible = true;

        switch (flags) {
        case DynHaveLastToolTerm:
            i->setEnabled(m_manager->lastToolTerm());
            break;
        case DynHaveLastToolServer:
            i->setEnabled(m_manager->lastToolServer());
            break;
        case DynLastToolName:
            i->setText(QCoreApplication::translate(m_id, "Active Tool") +
                       A(": ") + lastTool->windowTitle());
            i->setIcon(lastTool->action()->icon());
            break;
        case DynLastToolIsSearchable:
            visible = lastTool->searchable();
            break;
        case DynLastToolIsFilterable:
            visible = lastTool->filterable();
            break;
        case DynLastToolTableHeaderCheck:
            i->setChecked(lastTool->headerShown());
            visible = lastTool->hasHeader();
            break;
        case DynLastToolSearchBarCheck:
            i->setChecked(lastTool->barShown());
            // fallthru
        case DynLastToolDisplay:
            visible = lastTool->hasBar();
            break;
        case DynHistoryToolAndTerm:
            i->setEnabled(m_window->jobsWidget()->selectedTerm());
            break;
        case DynNotesToolAndTerm:
            i->setEnabled(m_window->notesWidget()->selectedTerm());
            break;
        case DynTasksToolAndTask:
            i->setEnabled(m_window->tasksWidget()->selectedTask());
            break;
        case DynTasksToolAndCancel:
            i->setEnabled(m_window->tasksWidget()->selectedTaskCancelable());
            break;
        case DynTasksToolAndClone:
            i->setEnabled(m_window->tasksWidget()->selectedTaskClonable());
            break;
        case DynTasksToolAndFileHide:
        case DynTasksToolOpenFile:
            visible = !m_window->tasksWidget()->selectedTaskFile().isEmpty();
            break;
        case DynFilesToolAndDir:
            i->setEnabled(m_window->filesWidget()->isCurrentDirectory());
            break;
        case DynFilesToolAndFile:
        case DynFilesToolOpenFile:
            i->setEnabled(m_window->filesWidget()->isSelectedFile());
            break;
        case DynFilesToolAndFileHide:
            visible = m_window->filesWidget()->isSelectedFile();
            break;
        case DynFilesToolAndRemoteDir:
            i->setEnabled(m_window->filesWidget()->isCurrentDirectory() &&
                          (m_window->filesWidget()->isRemote() || g_global->localDownload()));
            break;
        case DynFilesToolAndRemoteFile:
            i->setEnabled(m_window->filesWidget()->isSelectedFile() &&
                          (m_window->filesWidget()->isRemote() || g_global->localDownload()));
            break;
        case DynFilesToolFormatShort:
            i->setChecked(m_window->filesWidget()->format() != FilesLong);
            break;
        case DynFilesToolFormatLong:
            i->setChecked(m_window->filesWidget()->format() == FilesLong);
            break;
        }

        i->setVisible(visible);
    }
}

void
DynamicMenu::updateLaunchMenu()
{
    int k = 0;
    for (auto i: actions())
        if (AFLAGS(i) == DynLaunchItem)
            ++k;
        else
            break;

    for (int n = g_global->menuSize(); k < n; ++k) {
        auto a = QMenu::addAction(g_mtstr, this, SLOT(handleAction()));
        a->setDynFlag(DynLaunchItem);
    }
}

int
DynamicMenu::adjustLaunchItems(const QString &path, const QString &slot,
                               const QList<QAction*> &actions)
{
    QString tip(ACT_OPEN_FILE_ARG);
    QString text(QCoreApplication::translate("menu-tools-open", "Open with %1"));

    auto launchers = g_settings->getLaunchers(QUrl::fromLocalFile(path));
    int k = 0, n = launchers.size();

    for (auto i: actions)
        if (AFLAGS(i) != DynLaunchItem) {
            break;
        } else if (k < n) {
            const auto &elt = launchers[k++];
            adjustAction(i, text.arg(elt.first), tip.arg(elt.first), elt.second, slot + elt.first);
            i->setVisible(true);
        } else {
            i->setVisible(false);
        }

    return k;
}

void
DynamicMenu::preshowOpenFileMenu()
{
    auto filesWidget = m_window->filesWidget();
    const auto list = actions();

    int k = adjustLaunchItems(filesWidget->selectedUrl().path(), L("OpenFile|"), list);
    bool remote = filesWidget->isSelectedFile() && filesWidget->isRemote();

    for (int n = list.size(); k < n; ++k)
        if (AFLAGS(list[k]) == DynFilesToolMountableFile)
            list[k]->setEnabled(remote);
}

void
DynamicMenu::preshowOpenTaskMenu()
{
    auto tasksWidget = m_window->tasksWidget();
    adjustLaunchItems(tasksWidget->selectedTaskFile(), L("OpenTaskFile|"), actions());
}

void
DynamicMenu::updateServerTermMenu()
{
    int k = actions().size();

    for (int n = g_global->menuSize(); k < n; ++k) {
        QMenu::addAction(g_mtstr, this, SLOT(handleAction()));
    }
}

void
DynamicMenu::preshowServerTermMenu()
{
    QString tip(ACT_NEW_TERMINAL_SERVER);
    QString slot("NewTerminal||");

    auto actions = this->actions();
    int i = 0, n = actions.size();
    for (const auto server: g_listener->servers()) {
        if (i == n)
            break;

        adjustAction(actions[i], server->fullname(), tip.arg(server->shortname()),
                     server->serverInfo()->nameIcon(), slot + server->idStr());
        actions[i]->setVisible(true);
        ++i;
    }

    for (; i < n; ++i)
        actions[i]->setVisible(false);
}

void
DynamicMenu::preshow()
{
    for (auto i: actions())
    {
        QString action = i->property(OBJPROP_SLOT).toString();
        if (action.isEmpty())
            continue;

        QString keymap, s = i->toolTip();
        TermShortcut result;
        bool found = m_manager->lookupShortcut(action, keymap, &result);

        if (!found)
        {
            // s += A(" - ") + TR_TEXT2.arg(keymap);
        }
        else if (keymap == g_str_DEFAULT_KEYMAP)
        {
            if (result.additional.isEmpty()) {
                s += " - " + result.expression;
            } else {
                s += " - %1 (%2)";
                s = s.arg(result.expression, result.additional);
            }
        }
        else
        {
            s += A(" - ") + TR_TEXT1.arg(result.expression, keymap);

            if (!result.additional.isEmpty()) {
                s += L(" (%3)").arg(result.additional);
            }
        }

        i->setStatusTip(s);
    }
}

bool
DynamicMenu::event(QEvent *event)
{
    if (event->type() == QEvent::StatusTip) {
        const auto *stEvent = static_cast<QStatusTipEvent*>(event);
        m_window->a_statusTipForwarder->setStatusTip(stEvent->tip());
        m_window->a_statusTipForwarder->showStatusText(nullptr);
    }

    return QMenu::event(event);
}
