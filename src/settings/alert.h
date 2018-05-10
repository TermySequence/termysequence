// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base.h"
#include "app/color.h"

#include <QRegularExpression>

class TermInstance;
struct TermJob;

enum AlertType {
    AlertActive, AlertIdle, AlertFinish, AlertSuccess, AlertFailure,
    AlertString, AlertRegex, AlertBell, AlertAString, AlertARegex,
    AlertNTypes
};

enum AlertNeeds {
    NeedsNone,
    NeedsActivity, NeedsJobs, NeedsContent, NeedsBell, NeedsAttr
};

class AlertSettings final: public SettingsBase
{
    Q_OBJECT
    Q_PROPERTY(QString condString READ condString WRITE setCondString)
    Q_PROPERTY(QString condAttr READ condAttr WRITE setCondAttr)
    Q_PROPERTY(QString paramProfile READ paramProfile WRITE setParamProfile)
    Q_PROPERTY(QString paramSlot READ paramSlot WRITE setParamSlot)
    Q_PROPERTY(QString paramLauncher READ paramLauncher WRITE setParamLauncher)
    Q_PROPERTY(QString paramMessage READ paramMessage WRITE setParamMessage)
    Q_PROPERTY(int condition READ condition WRITE setCondition)
    Q_PROPERTY(int idleTime READ idleTime WRITE setIdleTime)
    Q_PROPERTY(int paramFlashes READ paramFlashes WRITE setParamFlashes)
    Q_PROPERTY(bool actSwitch READ actSwitch WRITE setActSwitch)
    Q_PROPERTY(bool actPush READ actPush WRITE setActPush)
    Q_PROPERTY(bool actSlot READ actSlot WRITE setActSlot)
    Q_PROPERTY(bool actLaunch READ actLaunch WRITE setActLaunch)
    Q_PROPERTY(bool actNotify READ actNotify WRITE setActNotify)
    Q_PROPERTY(bool actInd READ actInd WRITE setActInd)
    Q_PROPERTY(bool actFlash READ actFlash WRITE setActFlash)
    Q_PROPERTY(bool actTerm READ actTerm WRITE setActTerm)
    Q_PROPERTY(bool actServer READ actServer WRITE setActServer)

private:
    // Non_properties
    AlertNeeds m_needs;
    QHash<TermInstance*,int> m_timers;
    QRegularExpression m_regex;
    std::string m_string;

    // Properties
    REFPROP(QString, condString, setCondString)
    REFPROP(QString, condAttr, setCondAttr)
    REFPROP(QString, paramProfile, setParamProfile)
    REFPROP(QString, paramSlot, setParamSlot)
    REFPROP(QString, paramLauncher, setParamLauncher)
    REFPROP(QString, paramMessage, setParamMessage)
    VALPROP(int, condition, setCondition)
    VALPROP(int, idleTime, setIdleTime)
    VALPROP(int, paramFlashes, setParamFlashes)
    VALPROP(bool, actSwitch, setActSwitch)
    VALPROP(bool, actPush, setActPush)
    VALPROP(bool, actSlot, setActSlot)
    VALPROP(bool, actLaunch, setActLaunch)
    VALPROP(bool, actNotify, setActNotify)
    VALPROP(bool, actInd, setActInd)
    VALPROP(bool, actFlash, setActFlash)
    VALPROP(bool, actTerm, setActTerm)
    VALPROP(bool, actServer, setActServer)

protected:
    void timerEvent(QTimerEvent *event);

public:
    AlertSettings(const QString &name, const QString &path);
    static void populateDefaults();

    // Non-properties
    inline AlertNeeds needs() const { return m_needs; }
    ColorName activeColor() const;
    QString condStr() const;
    QString actionStr(const char *property) const;
    QString slotStr(TermInstance *term) const;

    void setFavorite(unsigned favorite);

    inline bool active() const { return m_refcount > 1; }
    void takeReference();
    void putReference();

    void watch(TermInstance *term);
    void unwatch(TermInstance *term);
    void reportActivity(TermInstance *term);
    void reportJob(TermInstance *term, const TermJob *job) const;
    void reportContent(TermInstance *term, const std::string &str) const;
    void reportBell(TermInstance *term) const;
    void reportAttribute(TermInstance *term, const QString &key,
                         const QString &value) const;
};
