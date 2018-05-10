// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "state.h"
#include "basemacros.h"

static const char *const s_keys[] = {
    NULL,
    "MainWindow/Geometry%1",
    "MainWindow/State%1",
    "MainWindow/Layout%1",
    "MainWindow/Order%1",
    "JobsWidget/Columns%1",
    "JobsWidget/Settings%1",
    "NotesWidget/Columns%1",
    "NotesWidget/Settings%1",
    "TasksWidget/Columns%1",
    "FilesWidget/Columns%1",
    "FilesWidget/Settings%1",
    "SearchWidget/Settings%1",
    "State/FavoriteTerminalIcons",
    "State/FavoriteServerIcons",
    "KeymapsWindow/Geometry",
    "ProfilesWindow/Geometry",
    "LaunchersWindow/Geometry",
    "ConnectsWindow/Geometry",
    "ServersWindow/Geometry",
    "AlertsWindow/Geometry",
    "PortsWindow/Geometry",
    "RulesWindow/Geometry",
    "IconsWindow/Geometry",
    "OutputWindow/Geometry",
    "ProfileSettings/Geometry",
    "ProfileSettings/Category",
    "LauncherSettings/Geometry",
    "LauncherSettings/Category",
    "ConnectSettings/Geometry",
    "ConnectSettings/Category",
    "ServerSettings/Geometry",
    "ServerSettings/Category",
    "AlertSettings/Geometry",
    "AlertSettings/Category",
    "GlobalSettings/Geometry",
    "GlobalSettings/Category",
    "TerminalsWindow/Geometry",
    "TerminalsWindow/Settings",
    "TipsWindow/Geometry",
    "KeymapEditor/Geometry",
};

StateSettings *g_state;

static const SettingDef s_stateDefs[] = {
    { "State/DefaultProfile", "defaultProfile", QVariant::String,
      NULL, NULL, NULL
    },
    { "State/DefaultLauncher", "defaultLauncher", QVariant::String,
      NULL, NULL, NULL
    },
    { "State/FavoriteProfiles", "favoriteProfiles", QVariant::StringList,
      NULL, NULL, NULL
    },
    { "State/FavoriteConnections", "favoriteConns", QVariant::StringList,
      NULL, NULL, NULL
    },
    { "State/FavoriteLaunchers", "favoriteLaunchers", QVariant::StringList,
      NULL, NULL, NULL
    },
    { "State/FavoriteAlerts", "favoriteAlerts", QVariant::StringList,
      NULL, NULL, NULL
    },
    { "State/ActionHistory", "actionHistory", QVariant::StringList,
      NULL, NULL, NULL
    },
    { "State/SuppressSetupDialog", "suppressSetup", QVariant::Bool,
      NULL, NULL, NULL
    },
    { "State/ShowTipOfTheDay", "showTotd", QVariant::Bool,
      NULL, NULL, NULL
    },
    { NULL }
};

static SettingsBase::SettingsDef s_stateDef = {
    SettingsBase::Global, s_stateDefs
};

StateSettings::StateSettings(const QString &path) :
    SettingsBase(s_stateDef, path)
{
    // initDefaults
    m_suppressSetup = false;
    m_showTotd = false;
}

void
StateSettings::populateDefaults()
{
    auto &v = s_stateDef.defaults;
    v.insert(B("defaultProfile"), g_str_DEFAULT_PROFILE);
    v.insert(B("defaultLauncher"), g_str_DEFAULT_PROFILE);
    v.insert(B("favoriteProfiles"), QStringList(g_str_DEFAULT_PROFILE));
}

//
// Properties
//
DIRECT_SETTER(StateSettings::setDefaultProfile, defaultProfile, const QString &)
DIRECT_SETTER(StateSettings::setDefaultLauncher, defaultLauncher, const QString &)
DIRECT_SETTER(StateSettings::setFavoriteProfiles, favoriteProfiles, const QStringList &)
DIRECT_SETTER(StateSettings::setFavoriteConns, favoriteConns, const QStringList &)
DIRECT_SETTER(StateSettings::setFavoriteLaunchers, favoriteLaunchers, const QStringList &)
DIRECT_SETTER(StateSettings::setFavoriteAlerts, favoriteAlerts, const QStringList &)
DIRECT_SETTER(StateSettings::setActionHistory, actionHistory, const QStringList &)
DIRECT_SETTER(StateSettings::setSuppressSetup, suppressSetup, bool)
DIRECT_SETTER(StateSettings::setShowTotd, showTotd, bool)

//
// Direct
//
QByteArray
StateSettings::fetch(StateSettingsKey key, int index)
{
    QString str = s_keys[key];

    if (index != -1)
        str = str.arg(index);

    return value(str).toByteArray();
}

void
StateSettings::store(StateSettingsKey key, const QByteArray &value, int index)
{
    if (m_writable) {
        QString str = s_keys[key];

        if (index != -1)
            str = str.arg(index);

        setValue(str, value);
    }
}

QByteArray
StateSettings::fetchVersioned(StateSettingsKey key, unsigned version,
                              int index, int minlen)
{
    QByteArray bytes = fetch(key, index);

    if (bytes.size() < 2) {
        return QByteArray();
    }
    if ((((unsigned)bytes[1] << 8) | bytes[0]) != version) {
        return QByteArray();
    }
    if (bytes.remove(0, 2).size() < minlen) {
        return QByteArray();
    }

    return bytes;
}

void
StateSettings::storeVersioned(StateSettingsKey key, unsigned version,
                              QByteArray &value, int index)
{
    value.prepend(version >> 8);
    value.prepend(version & 0xff);
    store(key, value, index);
}

QStringList
StateSettings::fetchList(StateSettingsKey key)
{
    return value(QString(s_keys[key])).toStringList();
}

void
StateSettings::storeList(StateSettingsKey key, const QStringList &value)
{
    setValue(QString(s_keys[key]), value);
}
