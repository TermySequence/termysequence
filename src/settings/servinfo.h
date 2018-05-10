// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base.h"
#include "app/color.h"
#include "lib/uuid.h"

#include <QIcon>
#include <QDateTime>

class ConnectSettings;
class ProfileSettings;

class ServerSettings final: public SettingsBase
{
    Q_OBJECT
    // Visible
    Q_PROPERTY(QStringList startup READ startup WRITE setStartup)
    Q_PROPERTY(QStringList ports READ ports WRITE setPorts NOTIFY portsChanged)
    Q_PROPERTY(QString defaultProfile READ defaultProfile WRITE setDefaultProfile)
    Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)
    Q_PROPERTY(QString downloadLocation READ downloadLocation WRITE setDownloadLocation)
    Q_PROPERTY(int downloadConfig READ downloadConfig WRITE setDownloadConfig)
    Q_PROPERTY(int uploadConfig READ uploadConfig WRITE setUploadConfig)
    Q_PROPERTY(int deleteConfig READ deleteConfig WRITE setDeleteConfig)
    Q_PROPERTY(int renameConfig READ renameConfig WRITE setRenameConfig)
    Q_PROPERTY(int renderImages READ renderImages WRITE setRenderImages)
    Q_PROPERTY(int allowLinks READ allowLinks WRITE setAllowLinks)
    // Hidden
    Q_PROPERTY(QString user READ user WRITE setUser)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString host READ host WRITE setHost)
    Q_PROPERTY(QString connName READ connName WRITE setConnName)
    Q_PROPERTY(QString lastIcon READ lastIcon WRITE setLastIcon)
    Q_PROPERTY(int connType READ connType WRITE setConnType)
    Q_PROPERTY(bool connPrimary READ connPrimary WRITE setConnPrimary)
    Q_PROPERTY(QDateTime lastTime READ lastTime WRITE setLastTime)

private:
    // Non_properties
    Tsq::Uuid m_id;
    bool m_active = false;
    QIcon m_connIcon;
    ColorName m_activeColor;
    QIcon m_activeIcon;
    QIcon m_nameIcon;

    // Properties
    REFPROP(QStringList, startup, setStartup)
    REFPROP(QStringList, ports, setPorts)
    REFPROP(QString, defaultProfile, setDefaultProfile)
    REFPROP(QString, icon, setIcon)
    REFPROP(QString, downloadLocation, setDownloadLocation)
    REFPROP(QString, user, setUser)
    REFPROP(QString, name, setName)
    REFPROP(QString, host, setHost)
    REFPROP(QString, connName, setConnName)
    REFPROP(QString, lastIcon, setLastIcon)
    REFPROP(QDateTime, lastTime, setLastTime)
    VALPROP(int, downloadConfig, setDownloadConfig)
    VALPROP(int, uploadConfig, setUploadConfig)
    VALPROP(int, deleteConfig, setDeleteConfig)
    VALPROP(int, renameConfig, setRenameConfig)
    VALPROP(int, renderImages, setRenderImages)
    VALPROP(int, allowLinks, setAllowLinks)
    VALPROP(int, connType, setConnType)
    VALPROP(bool, connPrimary, setConnPrimary)

signals:
    void portsChanged();
    void iconChanged(QString icon, bool priority);

    void fullnameChanged(QString fullname);
    void shortnameChanged(QString shortname);

public:
    ServerSettings(const Tsq::Uuid &id, const QString &path = QString());
    static void populateDefaults();

    void loadSettings();

    // Non-properties
    inline const Tsq::Uuid& id() const { return m_id; }
    inline const QString& idStr() const { return SettingsBase::m_name; }
    QString shortname() const;
    QString longname() const;
    QString fullname() const;

    inline const QIcon& connIcon() const { return m_connIcon; }

    inline bool active() const { return m_active; }
    inline ColorName activeColor() const { return m_activeColor; }
    inline const QIcon& activeIcon() const { return m_activeIcon; }
    inline const QIcon& nameIcon() const { return m_nameIcon; }
    void setActive(const ConnectSettings *conn, bool primary);
    void setInactive();

    ProfileSettings* profile(QString profileName);
};

inline void
ServerSettings::setConnName(const QString &connName)
{
    m_connName = connName;
}

inline void
ServerSettings::setConnPrimary(bool connPrimary)
{
    m_connPrimary = connPrimary;
}

inline void
ServerSettings::setLastTime(const QDateTime &lastTime)
{
    m_lastTime = lastTime;
}
