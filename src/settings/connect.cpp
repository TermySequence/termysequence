// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "app/enums.h"
#include "app/icons.h"
#include "connect.h"
#include "basemacros.h"
#include "settings.h"
#include "servinfo.h"
#include "launcher.h"
#include "choicewidget.h"
#include "checkwidget.h"
#include "intwidget.h"
#include "commandwidget.h"
#include "inputwidget.h"
#include "envwidget.h"
#include "serverwidget.h"

// Note: Each subtype group must be in ascending order
// Note: Batch and SSH entries must not move
static const ChoiceDef s_typeArg[] = {
    { TN("settings-enum", "Local Persistent"),
      Tsqt::ConnectionPersistent, ICON_CONNTYPE_PERSISTENT },
    { TN("settings-enum", "Local Transient"),
      Tsqt::ConnectionTransient, ICON_CONNTYPE_TRANSIENT },
    { TN("settings-enum", "Batch Connection"),
      Tsqt::ConnectionBatch, ICON_CONNTYPE_BATCH },
    { TN("settings-enum", "Remote Host (SSH)"),
      Tsqt::ConnectionSsh, ICON_CONNTYPE_SSH },
    { TN("settings-enum", "Container (machinectl)"),
      Tsqt::ConnectionMctl, ICON_CONNTYPE_CONTAINER },
    { TN("settings-enum", "Container (docker)"),
      Tsqt::ConnectionDocker, ICON_CONNTYPE_CONTAINER },
    { TN("settings-enum", "Container (kubectl)"),
      Tsqt::ConnectionKubectl, ICON_CONNTYPE_CONTAINER },
    { TN("settings-enum", "Container (rkt)"),
      Tsqt::ConnectionRkt, ICON_CONNTYPE_CONTAINER },
    { TN("settings-enum", "Switch User (sudo)"),
      Tsqt::ConnectionUserSudo, ICON_CONNTYPE_USER },
    { TN("settings-enum", "Switch User (su)"),
      Tsqt::ConnectionUserSu, ICON_CONNTYPE_USER },
    { TN("settings-enum", "Switch User (machinectl)"),
      Tsqt::ConnectionUserMctl, ICON_CONNTYPE_USER },
    { TN("settings-enum", "Switch User (pkexec)"),
      Tsqt::ConnectionUserPkexec, ICON_CONNTYPE_USER },
    { TN("settings-enum", "Other/Generic"),
      Tsqt::ConnectionGeneric, ICON_CONNTYPE_GENERIC },
    { NULL }
};

const ChoiceDef *const ConnectSettings::g_typeDescFull = s_typeArg;
const ChoiceDef *const ConnectSettings::g_typeDesc = s_typeArg + 3;

static const SettingDef s_connectDefs[] = {
    { "Type/Type", "type", QVariant::Int,
      TN("settings-category", "Type"),
      TN("settings", "Type"),
      new ChoiceWidgetFactory(ConnectSettings::g_typeDesc)
    },
    { "Server/LaunchFrom", "server", QVariant::String,
      TN("settings-category", "Server"),
      TN("settings", "Server to launch the connection from"),
      new ServerWidgetFactory
    },
    { "Connection/Command", "command", QVariant::StringList,
      TN("settings-category", "Connection"),
      TN("settings", "Command"),
      new CommandWidgetFactory
    },
    { "Connection/Directory", "directory", QVariant::String,
      TN("settings-category", "Connection"),
      TN("settings", "Directory"),
      new InputWidgetFactory
    },
    { "Connection/Environment", "environ", QVariant::StringList,
      TN("settings-category", "Connection"),
      TN("settings", "Environment"),
      new EnvironWidgetFactory
    },
    { "Connection/UseRawProtocol", "raw", QVariant::Bool,
      TN("settings-category", "Connection"),
      TN("settings", "Use raw protocol encoding"),
      new CheckWidgetFactory
    },
    { "Connection/UseLocalPty", "pty", QVariant::Bool,
      TN("settings-category", "Connection"),
      TN("settings", "Run command in a local pty"),
      new CheckWidgetFactory
    },
    { "Connection/KeepaliveTime", "keepalive", QVariant::UInt,
      TN("settings-category", "Connection"),
      TN("settings", "Keep-alive timeout"),
      new IntWidgetFactory(IntWidget::Millis, 0, 86400000, 1000, true)
    },
    { "Batch/Contents", "batch", QVariant::StringList,
      NULL, NULL, NULL
    },
    { NULL }
};

static SettingsBase::SettingsDef s_connectDef = {
    SettingsBase::Connect, s_connectDefs
};

void
ConnectSettings::populateDefaults()
{
    auto &v = s_connectDef.defaults;
    v.insert(B("type"), Tsqt::ConnectionGeneric);
}

void
ConnectSettings::initDefaults(bool anonymous)
{
    m_typeStr = QCoreApplication::translate("settings-enum", "Other/Generic");
    m_typeIcon = QI(ICON_CONNTYPE_GENERIC);
    m_activeColor = DisconnFg;
    if (!anonymous)
        m_nameIcon = ICON_CONNTYPE_NAMED;

    // initDefaults
    m_keepalive = 0;
    m_type = Tsqt::ConnectionGeneric;
    m_raw = false;
    m_pty = false;
}

ConnectSettings::ConnectSettings(const QString &name, const QString &path) :
    SettingsBase(s_connectDef, path)
{
    m_name = name;
    initDefaults(false);
}

ConnectSettings::ConnectSettings(const QString &name, bool anonymous) :
    SettingsBase(s_connectDef),
    m_anonymous(anonymous)
{
    m_name = name;
    initDefaults(true);
}

static inline void
reportUpdate(int row)
{
    if (row != -1)
        emit g_settings->connectionUpdated(row);
}

/*
 * Non-properties
 */
int
ConnectSettings::category() const
{
    if (m_type == Tsqt::ConnectionSsh)
        return m_type;
    if (m_type >= Tsqt::ConnectionUserSudo && m_type <= Tsqt::ConnectionUserPkexec)
        return Tsqt::ConnectionUserSudo;
    if (m_type >= Tsqt::ConnectionDocker && m_type <= Tsqt::ConnectionMctl)
        return Tsqt::ConnectionDocker;

    return Tsqt::ConnectionGeneric;
}

int
ConnectSettings::reservedType() const
{
    return (m_reservedType != -1) ? m_reservedType : m_type;
}

void
ConnectSettings::setReservedType(int type)
{
    const auto *ptr = g_typeDescFull;
    switch (type) {
    case Tsqt::ConnectionBatch:
        ptr += 2;
        m_commandStr = A("  ");
        m_nameIcon = ThumbIcon::fromTheme(ptr->icon);
        m_isbatch = true;
        break;
    case Tsqt::ConnectionTransient:
        ++ptr;
        m_commandStr = ' ';
        // fallthru
    case Tsqt::ConnectionPersistent:
        m_reserved = true;
        m_reservedType = type;
        break;
    default:
        for (ptr = g_typeDesc; ptr->value.toInt() != type; ++ptr);
    }

    m_typeStr = QCoreApplication::translate("settings-enum", ptr->description);
    m_typeIcon = ThumbIcon::fromTheme(ptr->icon);
}

void
ConnectSettings::setActive(ServerInstance *server)
{
    m_active = !!server;
    m_activeColor = server ? ConnFg : DisconnFg;
    m_activeIcon = server ? ICON_CONNTYPE_ACTIVE : QIcon();
    m_activeServer = server;
    reportUpdate(m_row);
}

void
ConnectSettings::setFavorite(unsigned favorite)
{
    m_favorite = favorite;
    reportUpdate(m_row);
}

void
ConnectSettings::handleLaunchNameChanged(const QString &name)
{
    m_launchName = name;
    reportUpdate(m_row);
}

/*
 * Properties
 */
void
ConnectSettings::setCommand(const QStringList &command)
{
    if (m_command != command) {
        if (command.isEmpty() || command.at(0).isEmpty()) {
            m_command = QStringList();
            m_commandStr.clear();
        } else {
            m_command = command;
            m_commandStr = LaunchSettings::makeCommandStr(command);
        }
        emit settingChanged("command", m_command);
        reportUpdate(m_row);
    }
}

void
ConnectSettings::setServer(const QString &server)
{
    if (m_server != server) {
        m_server = server;

        disconnect(m_mocLaunch);
        m_launchName.clear();
        m_launchId = Tsq::Uuid();

        ServerSettings *info = nullptr;
        if (!server.isEmpty()) {
            m_launchId.parse(server.toLatin1().data());
            info = g_settings->server(m_launchId);
        }
        if (info) {
            info->activate();
            m_mocLaunch = connect(info, SIGNAL(shortnameChanged(QString)),
                                  SLOT(handleLaunchNameChanged(const QString&)));
            m_launchName = info->shortname();
        }

        emit settingChanged("server", server);
        reportUpdate(m_row);
    }
}

void
ConnectSettings::setType(int type)
{
    if (type < Tsqt::ConnectionBatch || type >= Tsqt::ConnectionNTypes)
        type = Tsqt::ConnectionGeneric;

    if (m_type != type) {
        m_type = type;
        setReservedType(type);

        emit settingChanged("type", type);
        reportUpdate(m_row);
        g_settings->reportConnIcon(this);
    }
}

REG_SETTER(ConnectSettings::setEnviron, environ, const QStringList &)
REG_SETTER(ConnectSettings::setDirectory, directory, const QString &)
REG_SETTER(ConnectSettings::setKeepalive, keepalive, unsigned)
REG_SETTER(ConnectSettings::setRaw, raw, bool)
REG_SETTER(ConnectSettings::setPty, pty, bool)
