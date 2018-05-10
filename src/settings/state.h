// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base.h"

enum StateSettingsKey {
    InvalidStateKey,
    WindowGeometryKey,
    WindowStateKey,
    WindowLayoutKey,
    WindowOrderKey,
    JobsColumnsKey,
    JobsSettingsKey,
    NotesColumnsKey,
    NotesSettingsKey,
    TasksColumnsKey,
    FilesColumnsKey,
    FilesSettingsKey,
    SearchSettingsKey,
    TermIconsKey,
    ServerIconsKey,
    KeymapsGeometryKey,
    ProfilesGeometryKey,
    LaunchersGeometryKey,
    ConnectsGeometryKey,
    ServersGeometryKey,
    AlertsGeometryKey,
    PortsGeometryKey,
    SwitchGeometryKey,
    IconGeometryKey,
    OutputGeometryKey,
    ProfileGeometryKey,
    ProfileCategoryKey,
    LauncherGeometryKey,
    LauncherCategoryKey,
    ConnectGeometryKey,
    ConnectCategoryKey,
    ServerGeometryKey,
    ServerCategoryKey,
    AlertGeometryKey,
    AlertCategoryKey,
    GlobalGeometryKey,
    GlobalCategoryKey,
    TermsGeometryKey,
    TermsSettingsKey,
    TipsGeometryKey,
    KeymapGeometryKey,
};

class StateSettings final: public SettingsBase
{
    Q_OBJECT
    // All Hidden
    Q_PROPERTY(QString defaultProfile READ defaultProfile WRITE setDefaultProfile)
    Q_PROPERTY(QString defaultLauncher READ defaultLauncher WRITE setDefaultLauncher)
    Q_PROPERTY(QStringList favoriteProfiles READ favoriteProfiles WRITE setFavoriteProfiles)
    Q_PROPERTY(QStringList favoriteConns READ favoriteConns WRITE setFavoriteConns)
    Q_PROPERTY(QStringList favoriteLaunchers READ favoriteLaunchers WRITE setFavoriteLaunchers)
    Q_PROPERTY(QStringList favoriteAlerts READ favoriteAlerts WRITE setFavoriteAlerts)
    Q_PROPERTY(QStringList actionHistory READ actionHistory WRITE setActionHistory)
    Q_PROPERTY(bool suppressSetup READ suppressSetup WRITE setSuppressSetup)
    Q_PROPERTY(bool showTotd READ showTotd WRITE setShowTotd)

public:
    StateSettings(const QString &path = QString());
    static void populateDefaults();

    // Properties
    REFPROP(QString, defaultProfile, setDefaultProfile)
    REFPROP(QString, defaultLauncher, setDefaultLauncher)
    REFPROP(QStringList, favoriteProfiles, setFavoriteProfiles)
    REFPROP(QStringList, favoriteConns, setFavoriteConns)
    REFPROP(QStringList, favoriteLaunchers, setFavoriteLaunchers)
    REFPROP(QStringList, favoriteAlerts, setFavoriteAlerts)
    REFPROP(QStringList, actionHistory, setActionHistory)
    VALPROP(bool, suppressSetup, setSuppressSetup)
    VALPROP(bool, showTotd, setShowTotd)

    // Direct
    QByteArray fetch(StateSettingsKey key, int index = -1);
    void store(StateSettingsKey key, const QByteArray &value, int index = -1);

    QByteArray fetchVersioned(StateSettingsKey key, unsigned version,
                              int index = -1, int minlen = -1);
    void storeVersioned(StateSettingsKey key, unsigned version,
                        QByteArray &value, int index = -1);

    QStringList fetchList(StateSettingsKey key);
    void storeList(StateSettingsKey key, const QStringList &value);
};

extern StateSettings *g_state;
