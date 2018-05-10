// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/enums.h"
#include "app/icons.h"
#include "servinfo.h"
#include "basemacros.h"
#include "settings.h"
#include "connect.h"
#include "startupwidget.h"
#include "profileselect.h"
#include "choicewidget.h"
#include "downloadwidget.h"
#include "portwidget.h"
#include "imagewidget.h"
#include "lib/enums.h"

static const ChoiceDef s_uploadConfig[] = {
    { TN("settings-enum", "Use global setting"), -1 },
    { TN("settings-enum", "Ask what to do"), Tsq::TaskAsk },
    { TN("settings-enum", "Overwrite without asking"), Tsq::TaskOverwrite },
    { TN("settings-enum", "Rename without asking"), Tsq::TaskRename },
    { TN("settings-enum", "Fail"), Tsq::TaskFail },
    { NULL }
};

static const ChoiceDef s_deleteConfig[] = {
    { TN("settings-enum", "Use global setting"), -1 },
    { TN("settings-enum", "Ask what to do"), Tsq::TaskAsk },
    { TN("settings-enum", "Ask for folders only"), Tsq::TaskAskRecurse },
    { TN("settings-enum", "Delete without asking"), Tsq::TaskOverwrite },
    { NULL }
};

static const ChoiceDef s_renameConfig[] = {
    { TN("settings-enum", "Use global setting"), -1 },
    { TN("settings-enum", "Ask what to do"), Tsq::TaskAsk },
    { TN("settings-enum", "Overwrite without asking"), Tsq::TaskOverwrite },
    { TN("settings-enum", "Fail"), Tsq::TaskFail },
    { NULL }
};

static const ChoiceDef s_renderConfig[] = {
    { TN("settings-enum", "Use global setting"), -1 },
    { TN("settings-enum", "Disabled"), 0 },
    { TN("settings-enum", "Enabled"), 1 },
    { NULL }
};

static const SettingDef s_serverDefs[] = {
    { "Server/StartupProfiles", "startup", QVariant::StringList,
      TN("settings-category", "Server"),
      TN("settings", "Terminals to launch once connected"),
      new StartupWidgetFactory
    },
    { "Server/DefaultProfile", "defaultProfile", QVariant::String,
      TN("settings-category", "Server"),
      TN("settings", "Default profile for this server"),
      new ProfileSelectFactory(false)
    },
    { "Server/PortForwardingRules", "ports", QVariant::StringList,
      TN("settings-category", "Server"),
      TN("settings", "Port forwarding configuration"),
      new PortsWidgetFactory
    },
    { "Server/RenderInlineImages", "renderImages", QVariant::Int,
      TN("settings-category", "Server"),
      TN("settings", "Enable rendering of inline images"),
      new ChoiceWidgetFactory(s_renderConfig)
    },
    { "Server/AllowSmartHyperlinks", "allowLinks", QVariant::Int,
      TN("settings-category", "Server"),
      TN("settings", "Enable remote hyperlink menus and actions"),
      new ChoiceWidgetFactory(s_renderConfig)
    },
    { "Appearance/FixedThumbnailIcon", "icon", QVariant::String,
      TN("settings-category", "Appearance"),
      TN("settings", "Show specific icon in thumbnail view"),
      new ImageWidgetFactory(ThumbIcon::ServerType)
    },
    { "Files/DownloadLocation", "downloadLocation", QVariant::String,
      TN("settings-category", "Files"),
      TN("settings", "Where to place downloaded files"),
      new DownloadWidgetFactory(1)
    },
    { "Files/DownloadFileConfirmation", "downloadConfig", QVariant::Int,
      TN("settings-category", "Files"),
      TN("settings", "When downloading to a file that already exists"),
      new ChoiceWidgetFactory(s_uploadConfig)
    },
    { "Files/UploadFileConfirmation", "uploadConfig", QVariant::Int,
      TN("settings-category", "Files"),
      TN("settings", "When uploading to a file that already exists"),
      new ChoiceWidgetFactory(s_uploadConfig)
    },
    { "Files/DeleteFileConfirmation", "deleteConfig", QVariant::Int,
      TN("settings-category", "Files"),
      TN("settings", "When removing a file"),
      new ChoiceWidgetFactory(s_deleteConfig)
    },
    { "Files/RenameFileConfirmation", "renameConfig", QVariant::Int,
      TN("settings-category", "Files"),
      TN("settings", "When renaming to a file that already exists"),
      new ChoiceWidgetFactory(s_renameConfig)
    },
    { "State/User", "user", QVariant::String,
      NULL, NULL, NULL
    },
    { "State/Name", "name", QVariant::String,
      NULL, NULL, NULL
    },
    { "State/Host", "host", QVariant::String,
      NULL, NULL, NULL
    },
    { "State/Icon", "lastIcon", QVariant::String,
      NULL, NULL, NULL
    },
    { "State/LastSeenTime", "lastTime", QVariant::DateTime,
      NULL, NULL, NULL
    },
    { "State/LastConnectionName", "connName", QVariant::String,
      NULL, NULL, NULL
    },
    { "State/LastConnectionType", "connType", QVariant::Int,
      NULL, NULL, NULL
    },
    { "State/LastConnectionPrimary", "connPrimary", QVariant::Bool,
      NULL, NULL, NULL
    },
    { NULL }
};

static SettingsBase::SettingsDef s_serverDef = {
    SettingsBase::Server, s_serverDefs
};

void
ServerSettings::populateDefaults()
{
    auto &v = s_serverDef.defaults;
    v.insert(B("startup"), QStringList({g_str_SERVER_PROFILE, g_str_SERVER_PROFILE}));
    v.insert(B("renderImages"), -1);
    v.insert(B("allowLinks"), -1);
    v.insert(B("downloadLocation"), g_str_CURRENT_PROFILE);
    v.insert(B("downloadConfig"), -1);
    v.insert(B("uploadConfig"), -1);
    v.insert(B("deleteConfig"), -1);
    v.insert(B("renameConfig"), -1);
    v.insert(B("connType"), Tsqt::ConnectionGeneric);
}

ServerSettings::ServerSettings(const Tsq::Uuid &id, const QString &path) :
    SettingsBase(s_serverDef, path),
    m_id(id)
{
    SettingsBase::m_name = QString::fromLatin1(id.str().c_str());
    m_activeColor = DisconnFg;

    m_user = g_str_unknown;
    m_name = g_str_unknown;
    m_host = g_str_unknown;

    // initDefaults
    m_downloadConfig = -1;
    m_uploadConfig = -1;
    m_deleteConfig = -1;
    m_renameConfig = -1;
    m_renderImages = -1;
    m_allowLinks = -1;

    m_connType = Tsqt::ConnectionGeneric;
    m_connPrimary = false;
}

void
ServerSettings::loadSettings()
{
    SettingsBase::loadSettings();

    // Set connIcon
    if (m_connName.isEmpty()) {
        m_connIcon = QIcon();
    } else {
        const auto *ptr = ConnectSettings::g_typeDescFull;
        while (ptr->value.toInt() != m_connType)
            ++ptr;

        m_connIcon = ThumbIcon::fromTheme(ptr->icon);
    }
}

static inline void
reportUpdate(int row)
{
    emit g_settings->serverUpdated(row);
}

/*
 * Non-properties
 */
QString
ServerSettings::shortname() const
{
    return L("%1@%2").arg(m_user, m_host);
}

QString
ServerSettings::longname() const
{
    return L("%1@%2/%3").arg(m_user, m_host, m_name);
}

QString
ServerSettings::fullname() const
{
   return m_connPrimary ?
       L("%1@%2/%3 [%4]").arg(m_user, m_host, m_name, m_connName) :
       L("%1@%2/%3").arg(m_user, m_host, m_name);
}

void
ServerSettings::setActive(const ConnectSettings *conn, bool primary)
{
    m_connIcon = conn->typeIcon();
    int connType = conn->reservedType();
    bool changed = false;

    if (m_connType != connType) {
        setValue(A("State/LastConnectionType"), m_connType = connType);
        changed = true;
    }
    if (m_connName != conn->name()) {
        setValue(A("State/LastConnectionName"), m_connName = conn->name());
        changed = true;
    }
    if (m_connPrimary != primary) {
        setValue(A("State/LastConnectionPrimary"), m_connPrimary = primary);
        changed = true;
    }

    if (changed) {
        emit fullnameChanged(fullname());
    }

    m_lastTime = QDateTime::currentDateTime();
    setValue(A("State/LastSeenTime"), m_lastTime);

    m_active = true;
    m_activeColor = ConnFg;
    m_activeIcon = ICON_CONNTYPE_ACTIVE;
    reportUpdate(m_row);
}

void
ServerSettings::setInactive()
{
    if (m_active) {
        m_active = false;
        m_activeColor = DisconnFg;
        m_activeIcon = QIcon();

        m_lastTime = QDateTime::currentDateTime();
        setValue(A("State/LastSeenTime"), m_lastTime);
        reportUpdate(m_row);
    }
}

ProfileSettings *
ServerSettings::profile(QString profileName)
{
    if (profileName.isEmpty() || profileName == g_str_SERVER_PROFILE)
        profileName = defaultProfile();

    return g_settings->profile(profileName);
}

/*
 * Properties
 */
void
ServerSettings::setUser(const QString &user)
{
    if (m_user != user) {
        m_user = user;
        setValue(A("State/User"), user);
        emit fullnameChanged(fullname());
        emit shortnameChanged(shortname());
        reportUpdate(m_row);
    }
}

void
ServerSettings::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        setValue(A("State/Name"), name);
        emit fullnameChanged(fullname());
        reportUpdate(m_row);
    }
}

void
ServerSettings::setHost(const QString &host)
{
    if (m_host != host) {
        m_host = host;
        setValue(A("State/Host"), host);
        emit fullnameChanged(fullname());
        emit shortnameChanged(shortname());
        reportUpdate(m_row);
    }
}

void
ServerSettings::setIcon(const QString &icon)
{
    if (m_icon != icon) {
        m_icon = icon;

        m_nameIcon = ThumbIcon::getIcon(ThumbIcon::ServerType,
                                        icon.isEmpty() ? m_lastIcon : icon);

        emit iconChanged(icon, true);
        emit settingChanged("icon", icon);
        reportUpdate(m_row);
    }
}

void
ServerSettings::setLastIcon(const QString &lastIcon)
{
    if (m_lastIcon != lastIcon) {
        m_lastIcon = lastIcon;
        setValue(A("State/Icon"), lastIcon);

        m_nameIcon = ThumbIcon::getIcon(ThumbIcon::ServerType,
                                        m_icon.isEmpty() ? lastIcon : m_icon);

        reportUpdate(m_row);
    }
}

REPORT_SETTER(ServerSettings::setStartup, startup, const QStringList &)
SIG_SETTER(ServerSettings::setPorts, ports, const QStringList &, portsChanged)
REPORT_SETTER(ServerSettings::setDefaultProfile, defaultProfile, const QString &)
REG_SETTER(ServerSettings::setDownloadLocation, downloadLocation, const QString &)
REG_SETTER(ServerSettings::setDownloadConfig, downloadConfig, int)
REG_SETTER(ServerSettings::setUploadConfig, uploadConfig, int)
REG_SETTER(ServerSettings::setDeleteConfig, deleteConfig, int)
REG_SETTER(ServerSettings::setRenameConfig, renameConfig, int)
REG_SETTER(ServerSettings::setRenderImages, renderImages, int)
REG_SETTER(ServerSettings::setAllowLinks, allowLinks, int)

void
ServerSettings::setConnType(int connType)
{
    if (connType < 0 || connType >= Tsqt::ConnectionNTypes)
        m_connType = Tsqt::ConnectionGeneric;
    else
        m_connType = connType;
}
