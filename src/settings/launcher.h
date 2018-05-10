// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base.h"

#include <QIcon>
#include <QUrl>

enum LaunchType {
    LaunchDefault, LaunchLocalCommand, LaunchRemoteCommand, LaunchLocalTerm,
    LaunchRemoteTerm, LaunchWriteCommand, LaunchNTypes
};
enum MountType {
    MountNone, MountReadWrite, MountReadOnly, MountNTypes
};
enum InOutType {
    InOutNone, InOutFile, InOutDialog, InOutCopy, InOutWrite, InOutAction,
    InOutNTypes
};

struct LaunchParams
{
    QString cmd, dir;
    QStringList env;
};

class LaunchSettings final: public SettingsBase
{
    Q_OBJECT
    Q_PROPERTY(QStringList command READ command WRITE setCommand)
    Q_PROPERTY(QStringList environ READ environ WRITE setEnviron)
    Q_PROPERTY(QString directory READ directory WRITE setDirectory)
    Q_PROPERTY(QString extensions READ extensions WRITE setExtensions)
    Q_PROPERTY(QString schemes READ schemes WRITE setSchemes)
    Q_PROPERTY(QString profile READ profile WRITE setProfile)
    Q_PROPERTY(QString icon READ icon WRITE setIcon)
    Q_PROPERTY(QString inputFile READ inputFile WRITE setInputFile)
    Q_PROPERTY(QString outputFile READ outputFile WRITE setOutputFile)
    Q_PROPERTY(int launchType READ launchType WRITE setLaunchType)
    Q_PROPERTY(int mountType READ mountType WRITE setMountType)
    Q_PROPERTY(int mountIdle READ mountIdle WRITE setMountIdle)
    Q_PROPERTY(int inputType READ inputType WRITE setInputType)
    Q_PROPERTY(int outputType READ outputType WRITE setOutputType)
    Q_PROPERTY(int outputConfig READ outputConfig WRITE setOutputConfig)
    Q_PROPERTY(bool fileDirectory READ fileDirectory WRITE setFileDirectory)

private:
    // Non_properties
    const bool m_config;
    bool m_wantsFile = false, m_wantsUrl = false;
    QString m_patternStr, m_commandStr;
    QIcon m_typeIcon, m_nameIcon;
    QStringList m_extList, m_schList;

    // Properties
    REFPROP(QStringList, command, setCommand)
    REFPROP(QStringList, environ, setEnviron)
    REFPROP(QString, directory, setDirectory)
    REFPROP(QString, extensions, setExtensions)
    REFPROP(QString, schemes, setSchemes)
    REFPROP(QString, profile, setProfile)
    REFPROP(QString, icon, setIcon)
    REFPROP(QString, inputFile, setInputFile)
    REFPROP(QString, outputFile, setOutputFile)
    VALPROP(int, launchType, setLaunchType)
    VALPROP(int, mountType, setMountType)
    VALPROP(int, mountIdle, setMountIdle)
    VALPROP(int, inputType, setInputType)
    VALPROP(int, outputType, setOutputType)
    VALPROP(int, outputConfig, setOutputConfig)
    VALPROP(bool, fileDirectory, setFileDirectory)

public:
    LaunchSettings(const QString &name = QString(), const QString &path = QString(),
                   bool config = true);
    static void populateDefaults();

    // Non-properties
    inline const QString& patternStr() const { return m_patternStr; }
    inline const QString& commandStr() const { return m_commandStr; }
    inline const QIcon& typeIcon() const { return m_typeIcon; }
    inline const QIcon& nameIcon() const { return m_nameIcon; }
    inline bool config() const { return m_config; }

    bool local() const;
    bool buffered() const;

    void setFavorite(unsigned favorite);
    void setDefault(bool isdefault);

    bool matches(const QUrl &url) const;
    LaunchParams getParams(const AttributeMap &subs, char sep = '\x1f') const;
    QString getText(const AttributeMap &subs) const;

    static QString makeCommandStr(QStringList cmdspec);
};

inline bool
LaunchSettings::local() const
{
    switch (launchType()) {
    case LaunchDefault:
    case LaunchLocalCommand:
    case LaunchLocalTerm:
        return true;
    default:
        return false;
    }
}

inline bool
LaunchSettings::buffered() const
{
    switch (outputType()) {
    case InOutCopy:
    case InOutWrite:
    case InOutAction:
        return true;
    default:
        return false;
    }
}
