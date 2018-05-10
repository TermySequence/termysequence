// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/logwindow.h"
#include "app/pluginwindow.h"
#include "manager.h"
#include "mainwindow.h"
#include "server.h"
#include "term.h"
#include "runtask.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "settings/profile.h"
#include "settings/keymap.h"
#include "settings/switchrule.h"
#include "settings/iconrule.h"
#include "settings/settingswindow.h"
#include "settings/profilewindow.h"
#include "settings/keymapwindow.h"
#include "settings/keymapeditor.h"
#include "settings/extractdialog.h"
#include "settings/connectwindow.h"
#include "settings/serverwindow.h"
#include "settings/alertwindow.h"
#include "settings/switcheditor.h"
#include "settings/iconeditor.h"
#include "settings/portwindow.h"
#include "settings/termwindow.h"
#include "settings/profiledialog.h"
#include "settings/launchwindow.h"
#include "settings/tipwindow.h"
#include "settings/aboutwindow.h"

#define TR_TITLE1 TL("window-title", "Global Settings")

void
TermManager::actionEditProfile(QString profileName)
{
    ProfileSettings *profile = nullptr;

    if (profileName.isEmpty()) {
        profile = m_term ? m_term->profile() : g_settings->defaultProfile();
    } else if (g_settings->haveProfile(profileName)) {
        profile = g_settings->profile(profileName);
    }

    if (profile)
        g_settings->profileWindow(profile)->bringUp();
}

void
TermManager::actionEditKeymap(QString keymapName)
{
    TermKeymap *keymap = nullptr;

    if (keymapName.isEmpty()) {
        keymap = m_term ? m_term->keymap() : g_settings->defaultKeymap();
    } else if (g_settings->haveKeymap(keymapName)) {
        keymap = g_settings->keymap(keymapName);
    }

    if (keymap)
        g_settings->keymapWindow(keymap)->bringUp();
}

void
TermManager::actionEditServer(QString serverId)
{
    ServerInstance *result = lookupServer(serverId);
    if (result)
        g_settings->serverWindow(result->serverInfo())->bringUp();
}

void
TermManager::actionSwitchProfile(QString profileName, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);

    if (result) {
        if (profileName == g_str_PROMPT_PROFILE) {
            auto *dialog = new ProfileDialog(result, m_parent);
            connect(dialog, &ProfileDialog::okayed, this, &TermManager::actionSwitchProfile);
            dialog->show();
        } else {
            result->setProfile(g_settings->profile(profileName));
        }
    }
}

void
TermManager::actionPushProfile(QString profileName, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);

    if (result) {
        if (profileName == g_str_PROMPT_PROFILE) {
            auto *dialog = new ProfileDialog(result, m_parent);
            connect(dialog, &ProfileDialog::okayed, this, &TermManager::actionPushProfile);
            dialog->show();
        } else {
            result->pushProfile(g_settings->profile(profileName));
        }
    }
}

void
TermManager::actionPopProfile(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        result->popProfile();
}

void
TermManager::actionSetAlert(QString alertName, QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        if (alertName == g_str_PROMPT_PROFILE) {
            auto *dialog = new AlertDialog(result, m_parent);
            connect(dialog, &AlertDialog::okayed, this, &TermManager::actionSetAlert);
            dialog->show();
        } else {
            AlertSettings *alert = g_settings->alert(alertName);
            if (alert)
                result->setAlert(alert);
        }
    }
}

void
TermManager::actionClearAlert(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result) {
        result->setAlert(nullptr);
        result->clearAlert();
    }
}

void
TermManager::actionManageProfiles()
{
    if (g_profilewin == nullptr)
        g_profilewin = new ProfilesWindow;

    g_profilewin->bringUp();
}

void
TermManager::actionExtractProfile(QString terminalId)
{
    TermInstance *result = lookupTerm(terminalId);
    if (result)
        (new ExtractDialog(result, this))->show();
}

void
TermManager::actionManageKeymaps()
{
    if (g_keymapwin == nullptr)
        g_keymapwin = new KeymapsWindow;

    g_keymapwin->bringUp();
}

void
TermManager::actionManageConnections()
{
    if (g_connwin == nullptr)
        g_connwin = new ConnectsWindow;

    g_connwin->bringUp();
}

void
TermManager::actionManagePortForwarding()
{
    if (g_portwin == nullptr)
        g_portwin = new PortsWindow;

    g_portwin->bringUp();
}

void
TermManager::actionManageTerminals()
{
    if (g_termwin == nullptr)
        g_termwin = new TermsWindow(this);

    g_termwin->bringUp();
}

void
TermManager::actionManageServers()
{
    if (g_serverwin == nullptr)
        g_serverwin = new ServersWindow;

    g_serverwin->bringUp();
}

void
TermManager::actionEditGlobalSettings()
{
    if (g_globalwin == nullptr) {
        g_globalwin = new SettingsWindow(g_global);
        g_globalwin->setWindowTitle(TR_TITLE1);
    }

    g_globalwin->bringUp();
}

void
TermManager::actionEditSwitchRules()
{
    if (g_switchwin == nullptr)
        g_switchwin = new SwitchEditor(g_switchrules);

    g_switchwin->bringUp();
}

void
TermManager::actionEditIconRules()
{
    if (g_iconwin == nullptr)
        g_iconwin = new IconEditor(g_iconrules);

    g_iconwin->bringUp();
}

void
TermManager::actionManagePlugins()
{
    if (g_pluginwin == nullptr)
        g_pluginwin = new PluginsWindow;

    g_pluginwin->bringUp();
}

void
TermManager::actionManageLaunchers()
{
    if (g_launchwin == nullptr)
        g_launchwin = new LaunchersWindow;

    g_launchwin->bringUp();
}

void
TermManager::actionManageAlerts()
{
    if (g_alertwin == nullptr)
        g_alertwin = new AlertsWindow;

    g_alertwin->bringUp();
}

void
TermManager::actionPrompt()
{
    auto *dialog = new SlotDialog(m_parent);
    connect(dialog, &SlotDialog::okayed, this, &TermManager::invokeSlot);
    dialog->show();
}

void
TermManager::actionNotifySend(QString summary, QString body)
{
    LocalCommandTask::notifySend(summary, body);
}

void
TermManager::actionEventLog()
{
    g_logwin->bringUp();
}

void
TermManager::actionTipOfTheDay()
{
    if (g_tipwin == nullptr)
        g_tipwin = new TipsWindow;

    g_tipwin->bringUp();
}

void
TermManager::actionHelpAbout()
{
    new AboutWindow();
}
