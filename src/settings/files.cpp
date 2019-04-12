// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/exception.h"
#include "app/logging.h"
#include "app/plugin.h"
#include "app/pluginwindow.h"
#include "base/fontbase.h"
#include "base/thumbicon.h"
#include "os/encoding.h"
#include "os/locale.h"
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
#include "keymapwindow.h"
#include "keymapeditor.h"
#include "profilewindow.h"
#include "launchwindow.h"
#include "connectwindow.h"
#include "serverwindow.h"
#include "alertwindow.h"
#include "portwindow.h"
#include "termwindow.h"
#include "switchrule.h"
#include "switcheditor.h"
#include "tipwindow.h"
#include "iconrule.h"
#include "iconeditor.h"
#include "lib/base64.h"

#include <QDir>
#include <zlib.h>
#include <unistd.h>
#include <fcntl.h>

TermSettings *g_settings;

TermSettings::TermSettings()
{
    // calculate some defaults
    m_defaultFont = FontBase::getDefaultFont();
    m_defaultLang = osGetLang();
    m_defaultEncoding.reset(new TermUnicoding());

    StateSettings::populateDefaults();
    GlobalSettings::populateDefaults();
    ProfileSettings::populateDefaults(m_defaultFont);
    LaunchSettings::populateDefaults();
    ServerSettings::populateDefaults();
    ConnectSettings::populateDefaults();
    AlertSettings::populateDefaults();

    try {
        createFolders();
        m_haveFolders = true;
        g_global = new GlobalSettings(m_baseDir.filePath(APP_CONFIG));
        g_state = new StateSettings(m_baseDir.filePath(APP_STATE));
    }
    catch (const std::exception &e) {
        qCCritical(lcSettings) << e.what();
        g_global = new GlobalSettings;
        g_state = new StateSettings;
    }

    g_global->loadSettings();
    g_state->loadSettings();

    if (m_haveFolders) {
        g_switchrules = new SwitchRuleset(m_baseDir.filePath(APP_SWITCHRULES));
        g_switchrules->load();
        g_iconrules = new IconRuleset(m_baseDir.filePath(APP_ICONRULES));
        g_iconrules->load();
    } else {
        g_switchrules = new SwitchRuleset();
        g_iconrules = new IconRuleset();
    }
}

void
TermSettings::loadFolders()
{
    rescanKeymaps();
    rescanProfiles();
    rescanLaunchers();
    rescanServers();
    rescanConnections();
    rescanAlerts();
    rescanThemes();
    rescanPlugins();

    connect(g_global, SIGNAL(menuSizeChanged()), SLOT(updateProfiles()));
    connect(g_global, SIGNAL(menuSizeChanged()), SLOT(updateConnections()));
    connect(g_global, SIGNAL(menuSizeChanged()), SLOT(updateAlerts()));
}

TermSettings::~TermSettings()
{
    m_closing = true;

    forDeleteAll(m_settingsWindowMap);
    forDeleteAll(m_keymapWindowMap);
    delete g_iconwin;
    delete g_switchwin;
    delete g_termwin;
    delete g_portwin;
    delete g_keymapwin;
    delete g_profilewin;
    delete g_launchwin;
    delete g_serverwin;
    delete g_connwin;
    delete g_alertwin;
    delete g_globalwin;
    delete g_pluginwin;
    delete g_tipwin;

    forDeleteAll(m_profileList);
    forDeleteAll(m_keymapList);
    forDeleteAll(m_launchList);
    forDeleteAll(m_connList);
    forDeleteAll(m_serverList);
    forDeleteAll(m_alertList);
    forDeleteAll(m_themeList);
    forDeleteAll(m_pluginList);
    delete g_iconrules;
    delete g_switchrules;
    delete g_state;
    delete g_global;
}

void
TermSettings::createFolders()
{
    QString homePath(getenv("HOME"));
    QString basePath(getenv("XDG_CONFIG_HOME"));
    m_dataPath = getenv("XDG_DATA_HOME");

    // Find user data dir
    if (m_dataPath.isEmpty()) {
        if (!homePath.isEmpty())
            m_dataPath = L("%1/.local/share/" APP_NAME).arg(homePath);
    } else {
        m_dataPath.append(L("/" APP_NAME));
    }
    // Find user config dir
    if (basePath.isEmpty()) {
        if (homePath.isEmpty())
            throw TsqException("Neither XDG_CONFIG_HOME nor HOME are defined");

        basePath = L("%1/.config").arg(homePath);
    }

    m_baseDir = QDir(basePath, g_mtstr, QDir::Name, QDir::Files|QDir::Readable);

    if (!m_baseDir.isAbsolute())
        throw TsqException("Configuration folder is not an absolute path");
    if (!m_baseDir.exists(APP_NAME) && !m_baseDir.mkpath(APP_NAME))
        throw TsqException("Failed to create configuration folder");
    if (!m_baseDir.cd(APP_NAME))
        throw TsqException("Failed to change to configuration folder");

    if (!m_baseDir.exists(APP_KEYMAPDIR) && !m_baseDir.mkdir(APP_KEYMAPDIR))
        throw TsqException("Failed to create directory " APP_KEYMAPDIR);
    if (!m_baseDir.exists(APP_PROFILEDIR) && !m_baseDir.mkdir(APP_PROFILEDIR))
        throw TsqException("Failed to create directory " APP_PROFILEDIR);
    if (!m_baseDir.exists(APP_LAUNCHDIR) && !m_baseDir.mkdir(APP_LAUNCHDIR))
        throw TsqException("Failed to create directory " APP_LAUNCHDIR);
    if (!m_baseDir.exists(APP_SERVERDIR) && !m_baseDir.mkdir(APP_SERVERDIR))
        throw TsqException("Failed to create directory " APP_SERVERDIR);
    if (!m_baseDir.exists(APP_CONNDIR) && !m_baseDir.mkdir(APP_CONNDIR))
        throw TsqException("Failed to create directory " APP_CONNDIR);
    if (!m_baseDir.exists(APP_ALERTDIR) && !m_baseDir.mkdir(APP_ALERTDIR))
        throw TsqException("Failed to create directory " APP_ALERTDIR);
    if (!m_baseDir.exists(APP_THEMEDIR) && !m_baseDir.mkdir(APP_THEMEDIR))
        throw TsqException("Failed to create directory " APP_THEMEDIR);

    m_keymapDir = m_baseDir;
    m_keymapDir.cd(APP_KEYMAPDIR);
    m_keymapDir.setNameFilters(QStringList(L("*" KEYMAP_EXT)));

    m_profileDir = m_baseDir;
    m_profileDir.cd(APP_PROFILEDIR);
    m_profileDir.setNameFilters(QStringList(L("*" PROFILE_EXT)));

    m_launcherDir = m_baseDir;
    m_launcherDir.cd(APP_LAUNCHDIR);
    m_launcherDir.setNameFilters(QStringList(L("*" LAUNCH_EXT)));

    m_serverDir = m_baseDir;
    m_serverDir.cd(APP_SERVERDIR);
    m_serverDir.setNameFilters(QStringList(L("*" SERVER_EXT)));

    m_connDir = m_baseDir;
    m_connDir.cd(APP_CONNDIR);
    m_connDir.setNameFilters(QStringList(L("*" CONN_EXT)));

    m_alertDir = m_baseDir;
    m_alertDir.cd(APP_ALERTDIR);
    m_alertDir.setNameFilters(QStringList(L("*" ALERT_EXT)));

    m_themeDir = m_baseDir;
    m_themeDir.cd(APP_THEMEDIR);
    m_themeDir.setNameFilters(QStringList(L("*" THEME_EXT)));
}

QString
TermSettings::avatarPath() const
{
    return m_baseDir.filePath(APP_AVATAR);
}

std::string
TermSettings::loadAvatar(const QString &id) const
{
    char inbuf[AVATAR_MAX_SIZE];
    unsigned char outbuf[WRITER_BUFSIZE];
    int fd;
    ssize_t rc;
    size_t got = 0;
    z_stream zs = {};
    QByteArray raw;
    std::string result;

    if (!m_haveFolders)
        goto out;
    if ((fd = open(pr(avatarPath()), O_RDONLY)) == -1)
        goto out;

    do {
        rc = read(fd, inbuf + got, sizeof(inbuf) - got);
        if (rc == -1)
            goto out2;
        if ((got += rc) == sizeof(inbuf)) {
            qCWarning(lcSettings, APP_AVATAR " exceeds max size %zu", sizeof(inbuf) - 1);
            goto out2;
        }
    } while (rc);

    if (!ThumbIcon::loadIcon(ThumbIcon::AvatarType, id, QByteArray::fromRawData(inbuf, got)))
        goto out2;
    if (deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK)
        goto out2;

    zs.avail_in = got;
    zs.next_in = (unsigned char *)inbuf;

    do {
        zs.avail_out = sizeof(outbuf);
        zs.next_out = outbuf;
        deflate(&zs, Z_FINISH);
        raw.append((char *)outbuf, sizeof(outbuf) - zs.avail_out);
    } while (zs.avail_out == 0);

    base64(raw.data(), raw.size(), result);
    deflateEnd(&zs);
out2:
    close(fd);
out:
    return result;
}

void
TermSettings::parseAvatar(const Tsq::Uuid &id, const char *buf, size_t len)
{
    z_stream zs = {};

    if (!g_global->renderAvatars() || len == 0 || inflateInit(&zs) != Z_OK)
        return;

    QString idStr = QString::fromLatin1(id.str().c_str());
    unsigned char outbuf[WRITER_BUFSIZE];
    int ret;

    QByteArray decoded = QByteArray::fromBase64(QByteArray::fromRawData(buf, len));
    QByteArray raw;

    zs.avail_in = decoded.size();
    zs.next_in = (unsigned char *)decoded.data();

    do {
        zs.avail_out = sizeof(outbuf);
        zs.next_out = outbuf;
        switch (ret = inflate(&zs, Z_NO_FLUSH)) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            goto out;
        }
        raw.append((char *)outbuf, sizeof(outbuf) - zs.avail_out);
    } while (zs.avail_out == 0);

    if (ret == Z_STREAM_END && ThumbIcon::loadIcon(ThumbIcon::AvatarType, idStr, raw))
        emit g_global->avatarsChanged();
out:
    inflateEnd(&zs);
}

bool
TermSettings::needAvatar(const QString &id)
{
    if (g_global->renderAvatars() && !m_avatars.contains(id)) {
        m_avatars.insert(id);
        return true;
    }
    return false;
}
