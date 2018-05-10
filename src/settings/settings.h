// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/uuid.h"

#include <QObject>
#include <QDir>
#include <QHash>
#include <QSet>
#include <QVector>
#include <QIcon>
#include <QUrl>
#include <memory>

namespace Tsq { class Unicoding; }
class SettingsBase;
class SettingsWindow;
class TermKeymap;
class ProfileSettings;
class LaunchSettings;
class ConnectSettings;
class ServerSettings;
class AlertSettings;
class ThemeSettings;
class KeymapEditor;
class Plugin;

class TermSettings final: public QObject
{
    Q_OBJECT

private:
    QDir m_baseDir;
    QDir m_keymapDir;
    QDir m_profileDir;
    QDir m_launcherDir;
    QDir m_connDir;
    QDir m_serverDir;
    QDir m_alertDir;
    QDir m_themeDir;
    QString m_dataPath;

    TermKeymap *m_defaultKeymap = nullptr;
    ProfileSettings *m_defaultProfile = nullptr;
    LaunchSettings *m_defaultLauncher;
    ConnectSettings *m_transientConn = nullptr;
    ConnectSettings *m_persistentConn = nullptr;

    QHash<QString,TermKeymap*> m_keymapMap;
    QVector<TermKeymap*> m_keymapList;
    QHash<TermKeymap*,KeymapEditor*> m_keymapWindowMap;
    QHash<QString,ProfileSettings*> m_profileMap;
    QVector<ProfileSettings*> m_profileList;
    QHash<SettingsBase*,SettingsWindow*> m_settingsWindowMap;
    QHash<QString,LaunchSettings*> m_launchMap;
    QVector<LaunchSettings*> m_launchList;
    QHash<QString,ConnectSettings*> m_connMap;
    QVector<ConnectSettings*> m_connList;
    QHash<Tsq::Uuid,ServerSettings*> m_serverMap;
    QVector<ServerSettings*> m_serverList;
    QHash<QString,AlertSettings*> m_alertMap;
    QVector<AlertSettings*> m_alertList;
    QHash<QString,ThemeSettings*> m_themeMap;
    QVector<ThemeSettings*> m_themeList;
    QVector<Plugin*> m_pluginList;

    int m_anonOffset = 2;
    bool m_haveFolders = false;
    bool m_closing = false;
    bool m_haveSecondaryProfiles;
    bool m_needAllProfiles;
    bool m_updatingProfiles = false;
    bool m_updatingConns = false;
    bool m_haveSecondaryAlerts;
    bool m_needAllAlerts;
    bool m_updatingAlerts = false;
    unsigned m_profileFavidx = 0;
    unsigned m_launchFavidx = 0;
    unsigned m_connFavidx = 0;
    unsigned m_alertFavidx = 0;

    typedef std::pair<QString,QIcon> SIPair;
    typedef QVector<SIPair> SIVector;
    SIVector m_primaryProfiles, m_allProfiles;
    SIVector m_primaryConns, m_menuConns;
    SIVector m_primaryAlerts, m_allAlerts;
    QHash<QString,QVector<ThemeSettings*>> m_themesByGroup;

    QString m_defaultFont, m_defaultLang;
    std::shared_ptr<Tsq::Unicoding> m_defaultEncoding;

    QSet<QString> m_avatars;

    void createFolders();
    bool validateName(const QString &name, bool exists, bool mustNotExist) const;
    void validateKeymapName(const QString &name) const;
    LaunchSettings* validateLauncherName(const QString &name) const;
    void validateConnName(const QString &name) const;
    void validateAlertName(const QString &name) const;
    void addLauncher(LaunchSettings *launcher, LaunchSettings *share);

private slots:
    void handleConnDestroyed(QObject *conn);
    void updateProfiles();
    void updateConnections();
    void updateAlerts();

signals:
    void keymapAdded();
    void keymapUpdated(int index);
    void keymapRemoved(int index);
    void profileAdded();
    void profileUpdated(int index);
    void profileRemoved(int index);
    void launcherAdded();
    void launcherUpdated(int index);
    void launcherReplaced(int index);
    void launcherRemoved(int index);
    void launcherReload();
    void connectionAdded(int index);
    void connectionUpdated(int index);
    void connectionRemoved(int index);
    void serverAdded();
    void serverUpdated(int index);
    void serverRemoved(int index);
    void alertAdded();
    void alertUpdated(int index);
    void alertRemoved(int index);

    void profilesChanged();
    void launchersChanged();
    void connectionsChanged();
    void alertsChanged();
    void themesChanged();
    void iconsChanged();

    void pluginsReloaded();
    void pluginReloaded(Plugin *plugin);
    void pluginUnloaded(Plugin *plugin);

public:
    TermSettings();
    ~TermSettings();
    void loadFolders();

    inline bool closing() const { return m_closing; }
    inline const QString& dataPath() const { return m_dataPath; }
    inline const QString& defaultFont() const { return m_defaultFont; }
    inline const QString& defaultLang() const { return m_defaultLang; }
    inline auto defaultEncoding() const { return m_defaultEncoding; }

    QString avatarPath() const;
    std::string loadAvatar(const QString &id) const;
    bool needAvatar(const QString &id);
    void parseAvatar(const Tsq::Uuid &id, const char *buf, size_t len);

    //
    // Keymap
    //
    void rescanKeymaps();
    KeymapEditor* keymapWindow(TermKeymap *keymap);

    TermKeymap* keymap(const QString &keymapName) const;
    inline const auto& keymaps() const { return m_keymapList; }
    inline TermKeymap* defaultKeymap() const { return m_defaultKeymap; }
    bool haveKeymap(const QString &keymapName) const;

    TermKeymap* newKeymap(const QString &name, const QString &parent);
    TermKeymap* cloneKeymap(TermKeymap *from, const QString &to, const QString &parent);
    TermKeymap* renameKeymap(TermKeymap *from, const QString &to, const QString &parent);
    TermKeymap* reparentKeymap(TermKeymap *keymap, const QString &parent);
    void deleteKeymap(TermKeymap *keymap);

    //
    // Profile
    //
    void rescanProfiles();
    SettingsWindow* profileWindow(ProfileSettings *profile);

    ProfileSettings* profile(const QString &profileName) const;
    inline const auto& profiles() const { return m_profileList; }
    inline ProfileSettings* defaultProfile() const { return m_defaultProfile; }
    const ProfileSettings* startupProfile() const;
    bool haveProfile(const QString &profileName) const;

    inline const SIVector& primaryProfiles() const { return m_primaryProfiles; }
    const SIVector& allProfiles();
    inline bool haveSecondaryProfiles() const { return m_haveSecondaryProfiles; }
    void reportProfileIcon(const ProfileSettings *sender);

    void setDefaultProfile(ProfileSettings *defaultProfile);
    void setFavoriteProfile(ProfileSettings *profile, bool isFavorite);

    bool validateProfileName(const QString &name, bool mustNotExist) const;
    ProfileSettings* newProfile(const QString &name);
    ProfileSettings* cloneProfile(ProfileSettings *from, const QString &to);
    ProfileSettings* renameProfile(ProfileSettings *from, const QString &to);
    void deleteProfile(ProfileSettings *profile);

    //
    // Launcher
    //
    void rescanLaunchers();
    SettingsWindow* launcherWindow(LaunchSettings *launcher);

    LaunchSettings* launcher(const QString &launcherName) const;
    inline const auto& launchers() const { return m_launchList; }

    SIPair defaultLauncher() const;
    SIVector allLaunchers() const;
    SIVector getLaunchers(const QUrl &url) const;

    void setDefaultLauncher(LaunchSettings *defaultLauncher);
    void setFavoriteLauncher(LaunchSettings *launcher, bool isFavorite);

    LaunchSettings* newLauncher(const QString &name);
    LaunchSettings* cloneLauncher(LaunchSettings *from, const QString &to);
    LaunchSettings* renameLauncher(LaunchSettings *from, const QString &to);
    void deleteLauncher(LaunchSettings *launcher);

    //
    // Connection
    //
    void rescanConnections();
    SettingsWindow* connWindow(ConnectSettings *conn);

    ConnectSettings* conn(const QString &connName) const;
    inline const auto& conns() const { return m_connList; }
    inline ConnectSettings* transientConn() const { return m_transientConn; };
    inline ConnectSettings* persistentConn() const { return m_persistentConn; };

    inline const SIVector& primaryConns() const { return m_primaryConns; }
    inline const SIVector& menuConns() const { return m_menuConns; }
    void reportConnIcon(const ConnectSettings *sender);
    void setFavoriteConn(ConnectSettings *conn, bool isFavorite);

    ConnectSettings* newConn(const QString &name, int type);
    ConnectSettings* cloneConn(ConnectSettings *from, const QString &to);
    ConnectSettings* renameConn(ConnectSettings *from, const QString &to);
    void deleteConn(ConnectSettings *conn);
    void registerConn(ConnectSettings *conn);

    //
    // Server
    //
    void rescanServers();
    SettingsWindow* serverWindow(ServerSettings *server);

    ServerSettings* server(const Tsq::Uuid &serverId) const;
    ServerSettings* server(const Tsq::Uuid &serverId, ConnectSettings *conn, bool primary);
    inline const auto& servers() const { return m_serverList; }

    void deleteServer(ServerSettings *server);

    //
    // Alert
    //
    void rescanAlerts();
    SettingsWindow* alertWindow(AlertSettings *alert);

    AlertSettings* alert(const QString &alertName) const;
    inline const auto& alerts() const { return m_alertList; }

    inline const SIVector& primaryAlerts() const { return m_primaryAlerts; }
    const SIVector& allAlerts();
    inline bool haveSecondaryAlerts() const { return m_haveSecondaryAlerts; }

    void setFavoriteAlert(AlertSettings *alert, bool isFavorite);

    AlertSettings* newAlert(const QString &name);
    AlertSettings* cloneAlert(AlertSettings *from, const QString &to);
    AlertSettings* renameAlert(AlertSettings *from, const QString &to);
    void deleteAlert(AlertSettings *alert);

    //
    // Theme
    //
    void rescanThemes();

    ThemeSettings* theme(const QString &name) const;
    inline const auto& themes() const { return m_themeList; }
    inline auto themeGroups() const { return m_themesByGroup.keys(); }
    inline auto groupThemes(const QString &group) const { return m_themesByGroup[group]; }

    bool validateThemeName(const QString &name, bool mustNotExist) const;
    ThemeSettings* saveTheme(const QString &name);
    ThemeSettings* renameTheme(ThemeSettings *from, const QString &to);
    void updateTheme(ThemeSettings *theme);
    void deleteTheme(ThemeSettings *theme);

    //
    // Plugin
    //
    void rescanPlugins();
    inline const auto& plugins() const { return m_pluginList; }

    void unloadPlugin(Plugin *plugin);
    void reloadPlugin(Plugin *plugin);
    void unloadFeature(const QString &pluginName, const QString &featureName);
};

extern TermSettings *g_settings;
