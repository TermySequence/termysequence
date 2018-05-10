// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base.h"
#include "app/color.h"
#include "lib/uuid.h"

#include <QIcon>

struct ChoiceDef;
class ServerInstance;
class ServerSettings;

class ConnectSettings final: public SettingsBase
{
    Q_OBJECT
    // Visible
    Q_PROPERTY(QStringList command READ command WRITE setCommand)
    Q_PROPERTY(QStringList environ READ environ WRITE setEnviron)
    Q_PROPERTY(QString directory READ directory WRITE setDirectory)
    Q_PROPERTY(QString server READ server WRITE setServer)
    Q_PROPERTY(unsigned keepalive READ keepalive WRITE setKeepalive)
    Q_PROPERTY(int type READ type WRITE setType)
    Q_PROPERTY(bool raw READ raw WRITE setRaw)
    Q_PROPERTY(bool pty READ pty WRITE setPty)
    // Hidden
    Q_PROPERTY(QStringList batch READ batch WRITE setBatch)

private:
    // Non_properties
    int m_reservedType = -1;
    QString m_commandStr;
    QString m_typeStr;
    QIcon m_typeIcon;
    bool m_active = false;
    bool m_anonymous = false;
    bool m_isbatch = false;
    ColorName m_activeColor;
    QIcon m_activeIcon;
    ServerInstance *m_activeServer = nullptr;
    QIcon m_nameIcon;
    QString m_launchName;
    Tsq::Uuid m_launchId;
    QMetaObject::Connection m_mocLaunch;

    // Properties
    REFPROP(QStringList, command, setCommand)
    REFPROP(QStringList, environ, setEnviron)
    REFPROP(QStringList, batch, setBatch)
    REFPROP(QString, directory, setDirectory)
    REFPROP(QString, server, setServer)
    VALPROP(unsigned, keepalive, setKeepalive)
    VALPROP(int, type, setType)
    VALPROP(bool, raw, setRaw)
    VALPROP(bool, pty, setPty)

private slots:
    void handleLaunchNameChanged(const QString &name);

private:
    void initDefaults(bool anonymous);

public:
    ConnectSettings(const QString &name, const QString &path);
    ConnectSettings(const QString &name = QString(), bool anonymous = true);
    static void populateDefaults();

    // Non-properties
    inline const QString& commandStr() const { return m_commandStr; }
    inline const auto& launchId() const { return m_launchId; }
    inline const QString& launchName() const { return m_launchName; }

    int reservedType() const;
    int category() const;
    inline bool isbatch() const { return m_isbatch; }
    inline const QString& typeStr() const { return m_typeStr; }
    inline const QIcon& typeIcon() const { return m_typeIcon; }
    void setReservedType(int type);

    inline bool active() const { return m_active; }
    inline ColorName activeColor() const { return m_activeColor; }
    inline const QIcon& activeIcon() const { return m_activeIcon; }
    inline ServerInstance* activeServer() const { return m_activeServer; }
    void setActive(ServerInstance *server);

    inline bool anonymous() const { return m_anonymous; }
    inline const QIcon& nameIcon() const { return m_nameIcon; }

    void setFavorite(unsigned favorite);

    static const ChoiceDef *const g_typeDesc;
    static const ChoiceDef *const g_typeDescFull;
};

inline void
ConnectSettings::setBatch(const QStringList &batch)
{
    m_batch = batch;
}
