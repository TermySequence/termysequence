// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/enums.h"
#include "app/exception.h"
#include "app/logging.h"
#include "app/plugin.h"
#include "settings.h"
#include "global.h"
#include "keymap.h"
#include "profile.h"
#include "launcher.h"
#include "connect.h"
#include "servinfo.h"
#include "alert.h"
#include "theme.h"
#include "state.h"
#include "settingswindow.h"
#include "keymapeditor.h"

#include <QDir>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

#include "app/defthemes.hpp"

#define TR_ERROR0 TL("error", "Invalid name. Names must:\n" \
    "- contain only alphanumeric, space, and _'@-(). characters\n" \
    "- start with alphanumeric and end with non-space")
#define TR_ERROR1 TL("error", "Item \"%1\" already exists")
#define TR_ERROR2 TL("error", "Cannot save configuration files")
#define TR_ERROR3 TL("error", "Keymap \"%1\" does not exist")
#define TR_ERROR5 TL("error", "Keymap \"%1\" cannot inherit itself")
#define TR_ERROR6 "Default item cannot be deleted"
#define TR_ERROR7 "Default item cannot be renamed"
#define TR_ERROR8 "Default item cannot be reparented"
#define TR_ERROR12 TL("error", "Name \"%1\" is reserved. Choose another name")
#define TR_ERROR14 "Reserved connections cannot be deleted"
#define TR_ERROR15 "Reserved connections cannot be renamed"
#define TR_ERROR17 "Built-in themes cannot be deleted"
#define TR_TITLE1 TL("window-title", "Edit Profile %1")
#define TR_TITLE2 TL("window-title", "Edit Launcher %1")
#define TR_TITLE3 TL("window-title", "Edit Connection %1")
#define TR_TITLE4 TL("window-title", "Edit Server %1")
#define TR_TITLE5 TL("window-title", "Edit Alert %1")

#define NAME_REGEXP "\\A\\w(?:[ \\w\\.'@\\(\\)-]*[\\w\\.'@\\(\\)-])?\\z"
static const QRegularExpression s_nameRe(L(NAME_REGEXP));

bool
TermSettings::validateName(const QString &name, bool exists, bool mustNotExist) const
{
    if (!s_nameRe.match(name).hasMatch()) {
        throw CodedException(TSQ_ERR_BADOBJECTNAME, TR_ERROR0);
    }
    if (exists && mustNotExist) {
        throw CodedException(TSQ_ERR_OBJECTEXISTS, TR_ERROR1.arg(name));
    }

    return exists;
}

static inline unsigned
favidx(const QStringList &list, const QString &name, unsigned base)
{
    int idx = list.indexOf(name) + 1;
    return idx ? base + idx : 0;
}

//
// Keymap
//
void
TermSettings::rescanKeymaps()
{
    QFileInfoList files;
    QString path;
    int row = m_keymapList.size();

    if (m_haveFolders) {
        m_keymapDir.refresh();
        files = m_keymapDir.entryInfoList();
        path = m_keymapDir.filePath(g_str_DEFAULT_KEYMAP + KEYMAP_EXT);
    }

    // Get default set up first since the others may reference it
    if (!m_defaultKeymap) {
        m_defaultKeymap = new TermKeymap(g_str_DEFAULT_KEYMAP, path, true);
        m_defaultKeymap->setRow(row++);
        m_keymapMap.insert(g_str_DEFAULT_KEYMAP, m_defaultKeymap);
        m_keymapList.append(m_defaultKeymap);
        m_defaultKeymap->activate();
    }

    for (auto &i: qAsConst(files)) {
        const QString name = i.completeBaseName();

        if (!s_nameRe.match(name).hasMatch()) {
            qCWarning(lcSettings) << "Keymap" << name << "violates naming rules, skipping";
        }
        else if (!m_keymapMap.contains(name)) {
            auto *keymap = new TermKeymap(name, i.absoluteFilePath());
            keymap->setRow(row++);
            m_keymapMap.insert(name, keymap);
            m_keymapList.append(keymap);
            emit keymapAdded();
        }
    }
}

TermKeymap *
TermSettings::keymap(const QString &keymapName) const
{
    auto i = m_keymapMap.constFind(keymapName);
    if (i != m_keymapMap.cend())
        return *i;

    qCWarning(lcSettings) << "Keymap" << keymapName << "not found";
    return m_defaultKeymap;
}

bool
TermSettings::haveKeymap(const QString &keymapName) const
{
    return m_keymapMap.contains(keymapName);
}

KeymapEditor *
TermSettings::keymapWindow(TermKeymap *keymap)
{
    auto i = m_keymapWindowMap.constFind(keymap);
    if (i == m_keymapWindowMap.cend())
        i = m_keymapWindowMap.insert(keymap, new KeymapEditor(keymap));

    return *i;
}

inline void
TermSettings::validateKeymapName(const QString &name) const
{
    validateName(name, m_keymapMap.contains(name), true);
}

TermKeymap *
TermSettings::newKeymap(const QString &name, const QString &parent)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }

    validateKeymapName(name);
    QString path = m_keymapDir.filePath(name + KEYMAP_EXT);
    TermKeymap *keymap = new TermKeymap(name, path);

    if (!parent.isEmpty()) {
        auto i = m_keymapMap.constFind(parent);
        if (i == m_keymapMap.cend()) {
            delete keymap;
            throw CodedException(TSQ_ERR_OBJECTNOTFOUND, TR_ERROR3.arg(parent));
        }
        keymap->reparent(*i);
    }

    keymap->save();

    keymap->setRow(m_keymapList.size());
    m_keymapMap.insert(name, keymap);
    m_keymapList.append(keymap);
    emit keymapAdded();
    return keymap;
}

TermKeymap *
TermSettings::cloneKeymap(TermKeymap *from, const QString &to, const QString &parent)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }

    validateKeymapName(to);
    QString path = m_keymapDir.filePath(to + KEYMAP_EXT);
    TermKeymap *keymap = new TermKeymap(to, path);
    decltype(m_keymapMap)::const_iterator i;

    from->copy(keymap);

    if (parent.isEmpty()) {
        keymap->reparent(nullptr);
    }
    else if (parent == keymap->name()) {
        delete keymap;
        throw CodedException(TSQ_ERR_OBJECTINHERIT, TR_ERROR5.arg(parent));
    }
    else if ((i = m_keymapMap.constFind(parent)) == m_keymapMap.cend()) {
        delete keymap;
        throw CodedException(TSQ_ERR_OBJECTNOTFOUND, TR_ERROR3.arg(parent));
    }
    else {
        keymap->reparent(*i);
    }

    keymap->save();

    keymap->setRow(m_keymapList.size());
    m_keymapMap.insert(to, keymap);
    m_keymapList.append(keymap);
    emit keymapAdded();
    return keymap;
}

void
TermSettings::deleteKeymap(TermKeymap *keymap)
{
    QString name = keymap->name();
    QString filename = name + KEYMAP_EXT;

    if (keymap->reserved()) {
        throw CodedException(TSQ_ERR_OBJECTREADONLY, TR_ERROR6);
    }

    m_keymapMap.remove(name);
    int idx = m_keymapList.indexOf(keymap);
    m_keymapList.removeAt(idx);
    for (int i = idx, n = m_keymapList.size(); i < n; ++i)
        m_keymapList[i]->adjustRow(-1);
    emit keymapRemoved(idx);
    delete m_keymapWindowMap.take(keymap);
    delete keymap;

    if (m_haveFolders && !m_keymapDir.remove(filename))
        qCWarning(lcCommand) << "Failed to remove" << m_keymapDir.filePath(filename);
}

TermKeymap *
TermSettings::renameKeymap(TermKeymap *from, const QString &to, const QString &parent)
{
    if (from->reserved()) {
        throw CodedException(TSQ_ERR_OBJECTREADONLY, TR_ERROR7);
    }

    TermKeymap *keymap = cloneKeymap(from, to, parent);
    deleteKeymap(from);
    return keymap;
}

TermKeymap *
TermSettings::reparentKeymap(TermKeymap *keymap, const QString &parent)
{
    if (keymap->reserved()) {
        throw CodedException(TSQ_ERR_OBJECTREADONLY, TR_ERROR8);
    }

    decltype(m_keymapMap)::const_iterator i;

    if (parent.isEmpty()) {
        keymap->reparent(nullptr);
        keymap->save();
    }
    else if (parent == keymap->name()) {
        throw CodedException(TSQ_ERR_OBJECTINHERIT, TR_ERROR5.arg(parent));
    }
    else if ((i = m_keymapMap.constFind(parent)) == m_keymapMap.cend()) {
        throw CodedException(TSQ_ERR_OBJECTNOTFOUND, TR_ERROR3.arg(parent));
    }
    else {
        keymap->reparent(*i);
        keymap->save();
    }

    return keymap;
}

//
// Profile
//
static bool
ProfileComparator(const ProfileSettings *a, const ProfileSettings *b)
{
    if (a->name() == g_str_DEFAULT_PROFILE)
        return true;
    if (b->name() == g_str_DEFAULT_PROFILE)
        return false;

    if (a->isdefault())
        return true;
    if (b->isdefault())
        return false;

    if (a->favorite() != b->favorite())
        return a->favorite() > b->favorite();

    return a->name() < b->name();
}

const TermSettings::SIVector &
TermSettings::allProfiles()
{
    if (m_needAllProfiles) {
        auto profiles = m_profileList;
        std::sort(profiles.begin(), profiles.end(), ProfileComparator);
        int n = m_primaryProfiles.size(), k = 0;

        m_updatingProfiles = true;

        for (auto p: profiles) {
            if (k++ < n)
                continue;

            p->activate();
            m_allProfiles.append(std::make_pair(p->name(), p->nameIcon()));
        }

        m_updatingProfiles = m_needAllProfiles = false;
    }

    return m_allProfiles;
}

void
TermSettings::updateProfiles()
{
    auto profiles = m_profileList;
    std::sort(profiles.begin(), profiles.end(), ProfileComparator);
    int n = g_global->menuSize(), k = 0;

    m_primaryProfiles.clear();
    m_updatingProfiles = true;

    for (auto p: profiles) {
        if (++k > n)
            break;

        p->activate();
        m_primaryProfiles.append(std::make_pair(p->name(), p->nameIcon()));
    }

    m_updatingProfiles = false;
    m_haveSecondaryProfiles = m_needAllProfiles = (k > n);
    m_allProfiles = m_primaryProfiles;
    emit profilesChanged();
}

void
TermSettings::reportProfileIcon(const ProfileSettings *sender)
{
    if (m_updatingProfiles)
        return;

    int i = 0;
    for (int n = m_primaryProfiles.size(); i < n; ++i) {
        auto &elt = m_primaryProfiles[i];
        if (elt.first == sender->name()) {
            m_allProfiles[i].second = elt.second = sender->nameIcon();
            emit profilesChanged();
            return;
        }
    }
    for (int n = m_allProfiles.size(); i < n; ++i) {
        auto &elt = m_allProfiles[i];
        if (elt.first == sender->name()) {
            elt.second = sender->nameIcon();
            emit profilesChanged();
            break;
        }
    }
}

void
TermSettings::rescanProfiles()
{
    QFileInfoList files;
    QString path, name;
    QStringList favorites = g_state->favoriteProfiles();
    ProfileSettings *profile;
    int row = m_profileList.size();

    if (m_haveFolders) {
        m_profileDir.refresh();
        files = m_profileDir.entryInfoList();
        path = m_profileDir.filePath(g_str_DEFAULT_PROFILE + PROFILE_EXT);
    }

    for (auto &i: qAsConst(files)) {
        name = i.completeBaseName();

        if (!s_nameRe.match(name).hasMatch()) {
            qCWarning(lcSettings) << "Profile" << name << "violates naming rules, skipping";
        }
        else if (!m_profileMap.contains(name)) {
            profile = new ProfileSettings(name, i.absoluteFilePath());
            profile->setFavorite(favidx(favorites, name, m_profileFavidx));
            profile->setRow(row++);
            m_profileMap.insert(name, profile);
            m_profileList.append(profile);
            emit profileAdded();
        }
    }

    if (!m_profileMap.contains(name = g_str_DEFAULT_PROFILE)) {
        profile = new ProfileSettings(name, path);
        profile->setFavorite(favidx(favorites, name, m_profileFavidx));
        profile->setRow(row++);
        m_profileMap.insert(name, profile);
        m_profileList.append(profile);
        emit profileAdded();
    }

    if (!m_defaultProfile) {
        auto i = m_profileMap.constFind(g_state->defaultProfile());
        m_defaultProfile = i != m_profileMap.cend() ? *i : m_profileMap[name];
        m_defaultProfile->setDefault(true);
    }

    m_profileFavidx += favorites.size();
    updateProfiles();
}

SettingsWindow *
TermSettings::profileWindow(ProfileSettings *profile)
{
    auto i = m_settingsWindowMap.constFind(profile);
    if (i == m_settingsWindowMap.cend()) {
        SettingsWindow *window = new SettingsWindow(profile);
        window->setWindowTitle(TR_TITLE1.arg(profile->name()));
        i = m_settingsWindowMap.insert(profile, window);
    }
    return *i;
}

bool
TermSettings::validateProfileName(const QString &name, bool mustNotExist) const
{
    return validateName(name, m_profileMap.contains(name), mustNotExist);
}

ProfileSettings *
TermSettings::profile(const QString &profileName) const
{
    if (profileName.isEmpty() || profileName == g_str_CURRENT_PROFILE)
        return m_defaultProfile;

    auto i = m_profileMap.constFind(profileName);
    if (i != m_profileMap.cend())
        return *i;

    qCWarning(lcSettings) << "Profile" << profileName << "not found";
    return m_defaultProfile;
}

bool
TermSettings::haveProfile(const QString &profileName) const
{
    return profileName.isEmpty() ||
        m_profileMap.contains(profileName) ||
        profileName == g_str_CURRENT_PROFILE;
}

const ProfileSettings *
TermSettings::startupProfile() const
{
    m_defaultProfile->activate();
    return m_defaultProfile;
}

void
TermSettings::setDefaultProfile(ProfileSettings *profile)
{
    if (m_defaultProfile != profile) {
        m_defaultProfile->setDefault(false);
        m_defaultProfile = profile;
        m_defaultProfile->setDefault(true);
        g_state->setDefaultProfile(profile->name());
        updateProfiles();
    }
}

void
TermSettings::setFavoriteProfile(ProfileSettings *profile, bool isFavorite)
{
    if (profile->isfavorite() != isFavorite) {
        QStringList favorites = g_state->favoriteProfiles();

        if (isFavorite)
            favorites.append(profile->name());
        else
            favorites.removeAll(profile->name());

        profile->setFavorite(isFavorite ? ++m_profileFavidx : 0);
        g_state->setFavoriteProfiles(favorites);
        updateProfiles();
    }
}

ProfileSettings *
TermSettings::newProfile(const QString &name)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }

    validateProfileName(name, true);
    QString path = m_profileDir.filePath(name + PROFILE_EXT);
    ProfileSettings *profile = new ProfileSettings(name, path);

    profile->activate();
    profile->saveSettings();

    profile->setRow(m_profileList.size());
    m_profileMap.insert(name, profile);
    m_profileList.append(profile);
    emit profileAdded();
    updateProfiles();
    return profile;
}

ProfileSettings *
TermSettings::cloneProfile(ProfileSettings *from, const QString &to)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }

    validateProfileName(to, true);
    QString path = m_profileDir.filePath(to + PROFILE_EXT);
    ProfileSettings *profile = new ProfileSettings(to, path);

    from->activate();
    from->copySettings(profile);
    profile->saveSettings();

    profile->setRow(m_profileList.size());
    m_profileMap.insert(to, profile);
    m_profileList.append(profile);
    emit profileAdded();
    updateProfiles();
    return profile;
}

void
TermSettings::deleteProfile(ProfileSettings *profile)
{
    QString name = profile->name();
    QString filename = name + PROFILE_EXT;

    if (profile->reserved()) {
        throw CodedException(TSQ_ERR_OBJECTREADONLY, TR_ERROR6);
    }

    setFavoriteProfile(profile, false);
    if (profile->isdefault())
        setDefaultProfile(m_profileMap[g_str_DEFAULT_PROFILE]);

    m_profileMap.remove(name);
    int idx = m_profileList.indexOf(profile);
    m_profileList.removeAt(idx);
    for (int i = idx, n = m_profileList.size(); i < n; ++i)
        m_profileList[i]->adjustRow(-1);
    emit profileRemoved(idx);
    delete m_settingsWindowMap.take(profile);
    delete profile;
    updateProfiles();

    if (m_haveFolders && !m_profileDir.remove(filename))
        qCWarning(lcCommand) << "Failed to remove" << m_profileDir.filePath(filename);
}

ProfileSettings *
TermSettings::renameProfile(ProfileSettings *from, const QString &to)
{
    if (from->reserved()) {
        throw CodedException(TSQ_ERR_OBJECTREADONLY, TR_ERROR7);
    }

    ProfileSettings *profile = cloneProfile(from, to);
    deleteProfile(from);
    return profile;
}

//
// Launcher
//
static bool
LauncherComparator(const LaunchSettings *a, const LaunchSettings *b)
{
    if (a->isdefault())
        return true;
    if (b->isdefault())
        return false;

    if (a->favorite() != b->favorite())
        return a->favorite() > b->favorite();

    return a->name() < b->name();
}

TermSettings::SIPair
TermSettings::defaultLauncher() const
{
    return std::make_pair(m_defaultLauncher->name(), m_defaultLauncher->nameIcon());
}

TermSettings::SIVector
TermSettings::allLaunchers() const
{
    auto launchers = m_launchList;
    std::sort(launchers.begin(), launchers.end(), LauncherComparator);

    SIVector result;
    for (auto l: launchers) {
        result.append(std::make_pair(l->name(), l->nameIcon()));
    }
    return result;
}

TermSettings::SIVector
TermSettings::getLaunchers(const QUrl &url) const
{
    QVector<LaunchSettings*> tmp;

    for (const auto l: m_launchList)
        if (!l->isdefault() && l->matches(url))
            tmp.append(l);

    std::sort(tmp.begin(), tmp.end(), LauncherComparator);

    SIVector result;
    for (int i = 0, n = qMin(tmp.size(), g_global->menuSize()); i < n; ++i) {
        result.append(std::make_pair(tmp[i]->name(), tmp[i]->nameIcon()));
    }

    return result;
}

void
TermSettings::rescanLaunchers()
{
    // Clear out the current map for this settings object
    for (auto l: m_launchList) {
        l->putReferenceAndDeanimate();
    }
    m_launchList.clear();
    m_launchMap.clear();

    QFileInfoList files;
    QString path, name;
    QStringList favorites = g_state->favoriteLaunchers();
    LaunchSettings *launcher;

    if (m_haveFolders) {
        m_launcherDir.refresh();
        files = m_launcherDir.entryInfoList();
        path = m_launcherDir.filePath(g_str_DEFAULT_PROFILE + LAUNCH_EXT);
    }

    for (auto &i: qAsConst(files)) {
        name = i.completeBaseName();

        if (!s_nameRe.match(name).hasMatch()) {
            qCWarning(lcSettings) << "Launcher" << name << "violates naming rules, skipping";
        }
        else if (!m_launchMap.contains(name)) {
            launcher = new LaunchSettings(name, i.absoluteFilePath());
            launcher->activate(); // always
            launcher->setFavorite(favidx(favorites, name, m_launchFavidx));
            m_launchMap.insert(name, launcher);
            m_launchList.append(launcher);
        }
    }

    if (!m_launchMap.contains(name = g_str_DEFAULT_PROFILE)) {
        launcher = new LaunchSettings(name, path);
        launcher->activate(); // always
        launcher->setFavorite(favidx(favorites, name, m_launchFavidx));
        m_launchMap.insert(name, launcher);
        m_launchList.append(launcher);
    }

    // Also load launchers from the system dir
    // System data dir
    QDir systemDir(L(PREFIX "/share/" APP_NAME "/launchers/"));
    if (systemDir.exists()) {
        systemDir.setNameFilters(QStringList(L("*" LAUNCH_EXT)));
        systemDir.setFilter(QDir::Files|QDir::Readable);
        files = systemDir.entryInfoList();
    }
    for (auto &i: qAsConst(files)) {
        const QString name = i.completeBaseName();

        if (s_nameRe.match(name).hasMatch() && !m_launchMap.contains(name)) {
            launcher = new LaunchSettings(name, i.absoluteFilePath(), false);
            launcher->activate(); // always
            launcher->setFavorite(favidx(favorites, name, m_launchFavidx));
            m_launchMap.insert(name, launcher);
            m_launchList.append(launcher);
        }
    }

    // Set the default launcher
    auto i = m_launchMap.constFind(g_state->defaultLauncher());
    launcher = i != m_launchMap.cend() ? *i : m_launchMap[name];
    m_defaultLauncher = launcher;
    m_defaultLauncher->setDefault(true);

    // Presort the list
    std::sort(m_launchList.begin(), m_launchList.end(), LauncherComparator);
    int row = 0;
    for (auto l: m_launchList)
        l->setRow(row++);

    m_launchFavidx += favorites.size();
    emit launcherReload();
    emit launchersChanged();
}

SettingsWindow *
TermSettings::launcherWindow(LaunchSettings *launcher)
{
    auto i = m_settingsWindowMap.constFind(launcher);
    if (i == m_settingsWindowMap.cend()) {
        SettingsWindow *window = new SettingsWindow(launcher);
        window->setWindowTitle(TR_TITLE2.arg(launcher->name()));
        i = m_settingsWindowMap.insert(launcher, window);
    }
    return *i;
}

LaunchSettings *
TermSettings::validateLauncherName(const QString &name) const
{
    validateName(name, false, false);
    auto i = m_launchMap.find(name);

    if (i == m_launchMap.end())
        return nullptr;
    else if (!(*i)->config())
        return *i;
    else
        throw CodedException(TSQ_ERR_OBJECTEXISTS, TR_ERROR1.arg(name));
}

LaunchSettings *
TermSettings::launcher(const QString &launcherName) const
{
    if (launcherName.isEmpty())
        return m_defaultLauncher;

    auto i = m_launchMap.constFind(launcherName);
    if (i != m_launchMap.cend())
        return *i;

    qCWarning(lcSettings) << "Launcher" << launcherName << "not found";
    return m_defaultLauncher;
}

void
TermSettings::setDefaultLauncher(LaunchSettings *launcher)
{
    if (m_defaultLauncher != launcher) {
        m_defaultLauncher->setDefault(false);
        m_defaultLauncher = launcher;
        m_defaultLauncher->setDefault(true);
        g_state->setDefaultLauncher(launcher->name());
        emit launchersChanged();
    }
}

void
TermSettings::setFavoriteLauncher(LaunchSettings *launcher, bool isFavorite)
{
    if (launcher->isfavorite() != isFavorite) {
        QStringList favorites = g_state->favoriteLaunchers();

        if (isFavorite)
            favorites.append(launcher->name());
        else
            favorites.removeAll(launcher->name());

        launcher->setFavorite(isFavorite ? ++m_launchFavidx : 0);
        g_state->setFavoriteLaunchers(favorites);
    }
}

void
TermSettings::addLauncher(LaunchSettings *launcher, LaunchSettings *share)
{
    m_launchMap[launcher->name()] = launcher;
    if (share) {
        launcher->setFavorite(share->favorite());
        if (share->isdefault()) {
            m_defaultLauncher = launcher;
            launcher->setDefault(true);
            emit launchersChanged();
        }
        int row = share->row();
        launcher->setRow(row);
        m_launchList[row] = launcher;
        emit launcherReplaced(row);
        share->putReferenceAndDeanimate();
    } else {
        launcher->setRow(m_launchList.size());
        m_launchList.append(launcher);
        emit launcherAdded();
    }
}

LaunchSettings *
TermSettings::newLauncher(const QString &name)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }

    LaunchSettings *share = validateLauncherName(name);
    QString path = m_launcherDir.filePath(name + LAUNCH_EXT);
    LaunchSettings *launcher = new LaunchSettings(name, path);

    launcher->activate();
    launcher->saveSettings();

    addLauncher(launcher, share);
    return launcher;
}

LaunchSettings *
TermSettings::cloneLauncher(LaunchSettings *from, const QString &to)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }

    LaunchSettings *share = validateLauncherName(to);
    QString path = m_launcherDir.filePath(to + LAUNCH_EXT);
    LaunchSettings *launcher = new LaunchSettings(to, path);

    from->copySettings(launcher);
    launcher->saveSettings();

    addLauncher(launcher, share);
    return launcher;
}

void
TermSettings::deleteLauncher(LaunchSettings *launcher)
{
    QString name = launcher->name();
    QString filename = name + LAUNCH_EXT;

    if (launcher->reserved()) {
        throw CodedException(TSQ_ERR_OBJECTREADONLY, TR_ERROR6);
    }

    setFavoriteLauncher(launcher, false);
    if (launcher->isdefault())
        setDefaultLauncher(m_launchMap[g_str_DEFAULT_PROFILE]);

    m_launchMap.remove(name);
    int idx = m_launchList.indexOf(launcher);
    m_launchList.removeAt(idx);
    for (int i = idx, n = m_launchList.size(); i < n; ++i)
        m_launchList[i]->adjustRow(-1);
    emit launcherRemoved(idx);
    delete m_settingsWindowMap.take(launcher);
    delete launcher;

    if (m_haveFolders && !m_launcherDir.remove(filename))
        qCWarning(lcCommand) << "Failed to remove" << m_launcherDir.filePath(filename);
}

LaunchSettings *
TermSettings::renameLauncher(LaunchSettings *from, const QString &to)
{
    if (from->reserved()) {
        throw CodedException(TSQ_ERR_OBJECTREADONLY, TR_ERROR7);
    }

    LaunchSettings *launcher = cloneLauncher(from, to);
    deleteLauncher(from);
    return launcher;
}

//
// Connection
//
static bool
ConnComparator(const ConnectSettings *a, const ConnectSettings *b)
{
    if (a->favorite() != b->favorite())
        return a->favorite() > b->favorite();

    if (a->reservedType() == Tsqt::ConnectionTransient)
        return false;
    if (b->reservedType() == Tsqt::ConnectionTransient)
        return true;
    if (a->reservedType() == Tsqt::ConnectionPersistent)
        return false;
    if (b->reservedType() == Tsqt::ConnectionPersistent)
        return true;

    return a->name() < b->name();
}

void
TermSettings::updateConnections()
{
    auto conns = m_connList;
    std::sort(conns.begin(), conns.end(), ConnComparator);
    int n = g_global->menuSize(), k = 0;

    m_primaryConns.clear();
    m_menuConns.clear();
    m_menuConns.resize(5);
    m_updatingConns = true;

    for (auto c: conns) {
        if (++k > n)
            break;

        c->activate();
        m_primaryConns.append(std::make_pair(c->name(), c->typeIcon()));
    }

    k = 0;
    int l = 2, m = 4;
    for (auto c: conns) {
        if (!c->isfavorite())
            break;

        c->activate();

        switch (c->category()) {
        case Tsqt::ConnectionSsh:
            if (k < 2)
                m_menuConns[k++] = std::make_pair(c->name(), c->typeIcon());
            break;
        case Tsqt::ConnectionDocker:
            if (l < 4)
                m_menuConns[l++] = std::make_pair(c->name(), c->typeIcon());
            break;
        case Tsqt::ConnectionUserSudo:
            if (m < 5)
                m_menuConns[m++] = std::make_pair(c->name(), c->typeIcon());
            break;
        }
    }

    m_updatingConns = false;
    emit connectionsChanged();
}

void
TermSettings::reportConnIcon(const ConnectSettings *sender)
{
    if (!m_updatingConns)
        for (auto &elt: m_primaryConns)
            if (elt.first == sender->name()) {
                elt.second = sender->typeIcon();
                emit connectionsChanged();
                break;
            }
}

void
TermSettings::rescanConnections()
{
    QFileInfoList files;
    QStringList favorites = g_state->favoriteConns();
    int row = m_anonOffset;

    if (!m_persistentConn) {
        QString name = g_str_PERSISTENT_CONN;
        m_persistentConn = new ConnectSettings(name, false);
        m_persistentConn->setReservedType(Tsqt::ConnectionPersistent);
        m_persistentConn->setFavorite(favidx(favorites, name, m_connFavidx));
        m_persistentConn->setRow(0);
        m_connMap.insert(name, m_persistentConn);
        m_connList.append(m_persistentConn);

        name = g_str_TRANSIENT_CONN;
        m_transientConn = new ConnectSettings(name, false);
        m_transientConn->setReservedType(Tsqt::ConnectionTransient);
        m_transientConn->setFavorite(favidx(favorites, name, m_connFavidx));
        m_transientConn->setRow(1);
        m_connMap.insert(name, m_transientConn);
        m_connList.append(m_transientConn);
    }

    if (m_haveFolders) {
        m_connDir.refresh();
        files = m_connDir.entryInfoList();
    }
    for (auto &i: qAsConst(files)) {
        const QString name = i.completeBaseName();

        if (!s_nameRe.match(name).hasMatch()) {
            qCWarning(lcSettings) << "File" << name << "violates naming rules, skipping";
        }
        else if (!m_connMap.contains(name)) {
            auto *conn = new ConnectSettings(name, i.absoluteFilePath());
            conn->setFavorite(favidx(favorites, name, m_connFavidx));
            conn->setRow(row);
            m_connMap.insert(name, conn);
            m_connList.insert(row, conn);
            ++row;
        }
    }

    for (int i = row, n = m_connList.size(); i < n; ++i)
        m_connList[i]->adjustRow(row - m_anonOffset);
    for (int i = m_anonOffset; i < row; ++i)
        emit connectionAdded(i);

    m_connFavidx += favorites.size();
    m_anonOffset = row;
    updateConnections();
}

SettingsWindow *
TermSettings::connWindow(ConnectSettings *conn)
{
    auto i = m_settingsWindowMap.constFind(conn);
    if (i == m_settingsWindowMap.cend()) {
        SettingsWindow *window = new SettingsWindow(conn);
        window->setWindowTitle(TR_TITLE3.arg(conn->name()));
        i = m_settingsWindowMap.insert(conn, window);
    }
    return *i;
}

inline void
TermSettings::validateConnName(const QString &name) const
{
    if (name == g_str_TRANSIENT_CONN || name == g_str_PERSISTENT_CONN)
        throw CodedException(TSQ_ERR_BADOBJECTNAME, TR_ERROR12.arg(name));

    validateName(name, m_connMap.contains(name), true);
}

ConnectSettings *
TermSettings::conn(const QString &connName) const
{
    auto i = m_connMap.constFind(connName);
    if (i != m_connMap.cend())
        return *i;

    qCWarning(lcSettings) << "Connection" << connName << "not found";
    return nullptr;
}

void
TermSettings::setFavoriteConn(ConnectSettings *conn, bool isFavorite)
{
    if (conn->isfavorite() != isFavorite) {
        QStringList favorites = g_state->favoriteConns();

        if (isFavorite)
            favorites.append(conn->name());
        else
            favorites.removeAll(conn->name());

        conn->setFavorite(isFavorite ? ++m_connFavidx : 0);
        g_state->setFavoriteConns(favorites);
        updateConnections();
    }
}

ConnectSettings *
TermSettings::newConn(const QString &name, int type)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }

    validateConnName(name);
    QString path = m_connDir.filePath(name + CONN_EXT);
    ConnectSettings *conn = new ConnectSettings(name, path);

    conn->activate();
    conn->setType(type);

    conn->setRow(m_anonOffset);
    m_connMap.insert(name, conn);
    for (int i = m_anonOffset, n = m_connList.size(); i < n; ++i)
        m_connList[i]->adjustRow(1);
    m_connList.insert(m_anonOffset, conn);
    emit connectionAdded(m_anonOffset++);
    updateConnections();
    return conn;
}

ConnectSettings *
TermSettings::cloneConn(ConnectSettings *from, const QString &to)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }
    if (from->reserved()) {
        throw CodedException(TSQ_ERR_OBJECTREADONLY, TR_ERROR15);
    }

    validateConnName(to);
    QString path = m_connDir.filePath(to + CONN_EXT);
    ConnectSettings *conn = new ConnectSettings(to, path);

    from->activate();
    from->copySettings(conn);
    conn->saveSettings();

    conn->setRow(m_anonOffset);
    m_connMap.insert(to, conn);
    for (int i = m_anonOffset, n = m_connList.size(); i < n; ++i)
        m_connList[i]->adjustRow(1);
    m_connList.insert(m_anonOffset, conn);
    emit connectionAdded(m_anonOffset++);
    updateConnections();
    return conn;
}

void
TermSettings::deleteConn(ConnectSettings *conn)
{
    QString name = conn->name();
    QString filename = name + CONN_EXT;

    if (conn->reserved()) {
        throw CodedException(TSQ_ERR_OBJECTREADONLY, TR_ERROR14);
    }

    setFavoriteConn(conn, false);

    auto i = m_connMap.constFind(name);
    if (i == m_connMap.cend())
        return; // anonymous

    m_connMap.erase(i);
    int idx = m_connList.indexOf(conn);
    m_connList.removeAt(idx);
    for (int i = idx, n = m_connList.size(); i < n; ++i)
        m_connList[i]->adjustRow(-1);
    emit connectionRemoved(idx);
    delete m_settingsWindowMap.take(conn);
    conn->putReferenceAndDeanimate();
    --m_anonOffset;
    updateConnections();

    if (m_haveFolders && !m_connDir.remove(filename))
        qCWarning(lcCommand) << "Failed to remove" << m_connDir.filePath(filename);
}

ConnectSettings *
TermSettings::renameConn(ConnectSettings *from, const QString &to)
{
    ConnectSettings *conn = cloneConn(from, to);
    deleteConn(from);
    return conn;
}

void
TermSettings::handleConnDestroyed(QObject *obj)
{
    // Unregister anonymous conn
    int idx = m_connList.indexOf(static_cast<ConnectSettings*>(obj), m_anonOffset);
    m_connList.removeAt(idx);
    emit connectionRemoved(idx);
}

//
// Server
//
void
TermSettings::rescanServers()
{
    QFileInfoList files;
    int row = m_serverList.size();

    if (m_haveFolders) {
        m_serverDir.refresh();
        files = m_serverDir.entryInfoList();
    }
    for (auto &i: qAsConst(files)) {
        const QString name = i.completeBaseName();
        Tsq::Uuid id;

        if (!id.parse(name.toLatin1().data())) {
            qCWarning(lcSettings) << "File" << name << "not a valid UUID, skipping";
        }
        else if (!m_serverMap.contains(id)) {
            auto *servinfo = new ServerSettings(id, i.absoluteFilePath());
            servinfo->setRow(row++);
            m_serverMap.insert(id, servinfo);
            m_serverList.append(servinfo);
            emit serverAdded();
        }
    }
}

SettingsWindow *
TermSettings::serverWindow(ServerSettings *servinfo)
{
    auto i = m_settingsWindowMap.constFind(servinfo);
    if (i == m_settingsWindowMap.cend()) {
        SettingsWindow *window = new SettingsWindow(servinfo);
        window->setWindowTitle(TR_TITLE4.arg(servinfo->host()));
        i = m_settingsWindowMap.insert(servinfo, window);
    }
    return *i;
}

ServerSettings *
TermSettings::server(const Tsq::Uuid &serverId) const
{
    auto i = m_serverMap.constFind(serverId);
    return i != m_serverMap.cend() ? *i : nullptr;
}

ServerSettings *
TermSettings::server(const Tsq::Uuid &serverId, ConnectSettings *conn, bool primary)
{
    ServerSettings *servinfo;
    auto i = m_serverMap.constFind(serverId);
    if (i == m_serverMap.cend()) {
        QString path;
        if (m_haveFolders)
            path = m_serverDir.filePath(QString::fromLatin1(serverId.str().c_str()) + SERVER_EXT);

        servinfo = new ServerSettings(serverId, path);
        servinfo->setRow(m_serverList.size());
        m_serverMap.insert(serverId, servinfo);
        m_serverList.append(servinfo);
        emit serverAdded();
    } else {
        servinfo = *i;
    }

    if (conn->anonymous()) {
        // Register anonymous conn
        conn->setRow(m_connList.size());
        m_connList.append(conn);
        connect(conn, SIGNAL(destroyed(QObject*)), SLOT(handleConnDestroyed(QObject*)));
        emit connectionAdded(m_connList.size() - 1);
    }

    servinfo->activate();
    servinfo->setActive(conn, primary);
    return servinfo;
}

void
TermSettings::deleteServer(ServerSettings *server)
{
    QString id = server->idStr();
    QString filename = id + SERVER_EXT;

    if (server->active())
        return;

    m_serverMap.remove(server->id());
    int idx = m_serverList.indexOf(server);
    m_serverList.removeAt(idx);
    for (int i = idx, n = m_serverList.size(); i < n; ++i)
        m_serverList[i]->adjustRow(-1);
    emit serverRemoved(idx);
    delete m_settingsWindowMap.take(server);
    server->putReferenceAndDeanimate();

    if (m_haveFolders && !m_serverDir.remove(filename))
        qCWarning(lcCommand) << "Failed to remove" << m_serverDir.filePath(filename);
}

//
// Alert
//
static bool
AlertComparator(const AlertSettings *a, const AlertSettings *b)
{
    if (a->favorite() != b->favorite())
        return a->favorite() > b->favorite();

    return a->name() < b->name();
}

const TermSettings::SIVector &
TermSettings::allAlerts()
{
    if (m_needAllAlerts) {
        auto alerts = m_alertList;
        std::sort(alerts.begin(), alerts.end(), AlertComparator);
        int n = m_primaryAlerts.size(), k = 0;

        m_updatingAlerts = true;

        for (auto a: alerts) {
            if (k++ < n)
                continue;

            a->activate();
            m_allAlerts.append(std::make_pair(a->name(), QIcon()));
        }

        m_updatingAlerts = m_needAllAlerts = false;
    }

    return m_allAlerts;
}

void
TermSettings::updateAlerts()
{
    auto alerts = m_alertList;
    std::sort(alerts.begin(), alerts.end(), AlertComparator);
    int n = g_global->menuSize(), k = 0;

    m_primaryAlerts.clear();
    m_updatingAlerts = true;

    for (auto a: alerts) {
        if (++k > n)
            break;

        a->activate();
        m_primaryAlerts.append(std::make_pair(a->name(), QIcon()));
    }

    m_updatingAlerts = false;
    m_haveSecondaryAlerts = m_needAllAlerts = (k > n);
    m_allAlerts = m_primaryAlerts;
    emit alertsChanged();
}

void
TermSettings::rescanAlerts()
{
    QFileInfoList files;
    QStringList favorites = g_state->favoriteAlerts();
    int row = m_alertList.size();

    if (m_haveFolders) {
        m_alertDir.refresh();
        files = m_alertDir.entryInfoList();
    }
    for (auto &i: qAsConst(files)) {
        const QString name = i.completeBaseName();

        if (!s_nameRe.match(name).hasMatch()) {
            qCWarning(lcSettings) << "File" << name << "violates naming rules, skipping";
        }
        else if (!m_alertMap.contains(name)) {
            auto *alert = new AlertSettings(name, i.absoluteFilePath());
            alert->setFavorite(favidx(favorites, name, m_alertFavidx));
            alert->setRow(row++);
            m_alertMap.insert(name, alert);
            m_alertList.append(alert);
            emit alertAdded();
        }
    }

    m_alertFavidx += favorites.size();
    updateAlerts();
}

SettingsWindow *
TermSettings::alertWindow(AlertSettings *alert)
{
    auto i = m_settingsWindowMap.constFind(alert);
    if (i == m_settingsWindowMap.cend()) {
        SettingsWindow *window = new SettingsWindow(alert);
        window->setWindowTitle(TR_TITLE5.arg(alert->name()));
        i = m_settingsWindowMap.insert(alert, window);
    }
    return *i;
}

inline void
TermSettings::validateAlertName(const QString &name) const
{
    validateName(name, m_alertMap.contains(name), true);
}

AlertSettings *
TermSettings::alert(const QString &alertName) const
{
    auto i = m_alertMap.constFind(alertName);
    if (i != m_alertMap.cend())
        return *i;

    qCWarning(lcSettings) << "Alert" << alertName << "not found";
    return nullptr;
}

void
TermSettings::setFavoriteAlert(AlertSettings *alert, bool isFavorite)
{
    if (alert->isfavorite() != isFavorite) {
        QStringList favorites = g_state->favoriteAlerts();

        if (isFavorite)
            favorites.append(alert->name());
        else
            favorites.removeAll(alert->name());

        alert->setFavorite(isFavorite ? ++m_alertFavidx : 0);
        g_state->setFavoriteAlerts(favorites);
        updateAlerts();
    }
}

AlertSettings *
TermSettings::newAlert(const QString &name)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }

    validateAlertName(name);
    QString path = m_alertDir.filePath(name + ALERT_EXT);
    AlertSettings *alert = new AlertSettings(name, path);

    alert->activate();
    alert->saveSettings();

    alert->setRow(m_alertList.size());
    m_alertMap.insert(name, alert);
    m_alertList.append(alert);
    emit alertAdded();
    updateAlerts();
    return alert;
}

AlertSettings *
TermSettings::cloneAlert(AlertSettings *from, const QString &to)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }

    validateAlertName(to);
    QString path = m_alertDir.filePath(to + ALERT_EXT);
    AlertSettings *alert = new AlertSettings(to, path);

    from->activate();
    from->copySettings(alert);
    alert->saveSettings();

    alert->setRow(m_alertList.size());
    m_alertMap.insert(to, alert);
    m_alertList.append(alert);
    emit alertAdded();
    updateAlerts();
    return alert;
}

void
TermSettings::deleteAlert(AlertSettings *alert)
{
    QString name = alert->name();
    QString filename = name + ALERT_EXT;

    setFavoriteAlert(alert, false);

    m_alertMap.remove(name);
    int idx = m_alertList.indexOf(alert);
    m_alertList.removeAt(idx);
    for (int i = idx, n = m_alertList.size(); i < n; ++i)
        m_alertList[i]->adjustRow(-1);
    emit alertRemoved(idx);
    delete m_settingsWindowMap.take(alert);
    alert->putReferenceAndDeanimate();
    updateAlerts();

    if (m_haveFolders && !m_alertDir.remove(filename))
        qCWarning(lcCommand) << "Failed to remove" << m_alertDir.filePath(filename);
}

AlertSettings *
TermSettings::renameAlert(AlertSettings *from, const QString &to)
{
    AlertSettings *alert = cloneAlert(from, to);
    deleteAlert(from);
    return alert;
}

//
// Theme
//
static bool
ThemeComparator(const ThemeSettings *a, const ThemeSettings *b)
{
    if (a->lesser() != b->lesser())
        return b->lesser();
    else if (a->order() == -1 && b->order() == -1)
        return a->name() < b->name();
    else
        return a->order() < b->order();
}

void
TermSettings::rescanThemes()
{
    // Clear out the current map for this settings object
    for (auto &&i: m_themeMap)
        i->putReference();
    m_themeMap.clear();
    m_themeList.clear();
    m_themesByGroup.clear();

    QFileInfoList files;

    if (m_haveFolders) {
        m_themeDir.refresh();
        files = m_themeDir.entryInfoList();
    }

    for (auto &i: qAsConst(files)) {
        const QString name = i.completeBaseName();

        if (!s_nameRe.match(name).hasMatch()) {
            qCWarning(lcSettings) << "Theme" << name << "violates naming rules, skipping";
        }
        else {
            auto *theme = new ThemeSettings(name, i.absoluteFilePath());
            theme->activate(); // unique among file types
            m_themeMap.insert(name, theme);
            m_themeList.append(theme);
            m_themesByGroup[theme->group()].append(theme);
        }
    }

    // Insert the default themes, if not already present
    for (int i = 0; i < NTHEMES; ++i) {
        const ThemeDef *ptr = s_defaultThemes + i;
        if (m_themeMap.contains(ptr->name))
            continue;

        ThemeSettings *theme = new ThemeSettings(ptr, i);
        m_themeMap.insert(ptr->name, theme);
        m_themeList.append(theme);
        m_themesByGroup[ptr->group].append(theme);
    }

    std::sort(m_themeList.begin(), m_themeList.end(), ThemeComparator);
    emit themesChanged();
}

bool
TermSettings::validateThemeName(const QString &name, bool mustNotExist) const
{
    return validateName(name, m_themeMap.contains(name), mustNotExist);
}

ThemeSettings *
TermSettings::theme(const QString &themeName) const
{
    auto i = m_themeMap.constFind(themeName);
    if (i != m_themeMap.cend())
        return *i;

    qCWarning(lcSettings) << "Theme" << themeName << "not found";
    return nullptr;
}

void
TermSettings::updateTheme(ThemeSettings *theme)
{
    if (m_themeMap.contains(theme->name())) {
        for (auto &group: m_themesByGroup)
            if (group.removeOne(theme))
                break;
    } else {
        m_themeMap.insert(theme->name(), theme);
        m_themeList.append(theme);
    }

    m_themesByGroup[theme->group()].append(theme);
    std::sort(m_themeList.begin(), m_themeList.end(), ThemeComparator);

    theme->saveSettings();
    emit themesChanged();
}

ThemeSettings *
TermSettings::saveTheme(const QString &name)
{
    if (!m_haveFolders) {
        throw CodedException(TSQ_ERR_CANNOTSAVE, TR_ERROR2);
    }

    auto i = m_themeMap.find(name);
    if (i != m_themeMap.cend())
    {
        if (!(*i)->builtin())
            return (*i);

        m_themeList.removeOne(*i);
        (*i)->putReference();
        m_themeMap.erase(i);
    }

    QString path = m_themeDir.filePath(name + THEME_EXT);
    ThemeSettings *theme = new ThemeSettings(name, path);

    // Caller must update the theme, then call updateTheme()
    theme->activate();
    return theme;
}

void
TermSettings::deleteTheme(ThemeSettings *theme)
{
    QString name = theme->name();
    QString filename = name + THEME_EXT;

    if (theme->builtin()) {
        throw CodedException(TSQ_ERR_OBJECTREADONLY, TR_ERROR17);
    }

    m_themeMap.remove(name);
    theme->putReference();

    if (m_haveFolders && !m_themeDir.remove(filename))
        qCWarning(lcCommand) << "Failed to remove" << m_themeDir.filePath(filename);

    rescanThemes();
}

ThemeSettings *
TermSettings::renameTheme(ThemeSettings *from, const QString &to)
{
    ThemeSettings *theme = saveTheme(to);
    from->copySettings(theme);
    updateTheme(theme);

    if (!from->builtin())
        deleteTheme(from);

    return theme;
}

//
// Plugin
//
void
TermSettings::rescanPlugins()
{
    if (!i) {
        QVariant v = qApp->property(OBJPROP_V8_ERROR);
        if (!v.isNull())
            qCCritical(lcPlugin, "Plugins disabled: %s", pr(v.toString()));
        return;
    }

    forDeleteAll(m_pluginList);
    m_pluginList.clear();

    QString userPath(m_dataPath + A("/plugins"));
    QString systemPath(L(PREFIX "/share/" APP_NAME "/plugins"));
    QString *paths[] = { &userPath, &systemPath };
    QSet<QString> filenames;

    int npaths = 1 + qApp->property(OBJPROP_V8_SYSPLUGINS).toBool();

    for (int i = 0; i < npaths; ++i)
    {
        QDir dir(*paths[i], A("*.mjs"), QDir::Name, QDir::Files|QDir::Readable);

        for (auto &i: dir.entryInfoList()) {
            const QString name = i.fileName();

            if (!filenames.contains(name)) {
                auto *plugin = new Plugin(i.absoluteFilePath(), name);
                m_pluginList.append(plugin);
                plugin->start();
                filenames.insert(name);
            }
        }
    }

    emit pluginsReloaded();
}

void
TermSettings::unloadPlugin(Plugin *plugin)
{
    m_pluginList.removeOne(plugin);
    emit pluginUnloaded(plugin);
    plugin->deleteLater();
}

void
TermSettings::reloadPlugin(Plugin *plugin)
{
    plugin->reload();
    emit pluginReloaded(plugin);
}

void
TermSettings::unloadFeature(const QString &pluginName, const QString &featureName)
{
    for (auto plugin: qAsConst(m_pluginList))
        if (plugin->name() == pluginName) {
            plugin->unloadFeature(featureName);
            break;
        }
}
