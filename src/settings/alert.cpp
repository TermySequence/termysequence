// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "alert.h"
#include "basemacros.h"
#include "settings.h"
#include "choicewidget.h"
#include "checkwidget.h"
#include "intwidget.h"
#include "inputwidget.h"
#include "profileselect.h"
#include "launchselect.h"
#include "slotwidget.h"
#include "base/term.h"
#include "base/server.h"
#include "base/job.h"

static const ChoiceDef s_condArg[] = {
    { TN("settings-enum", "Activity in terminal"), AlertActive },
    { TN("settings-enum", "Inactivity in terminal"), AlertIdle },
    { TN("settings-enum", "Command finishes in terminal"), AlertFinish },
    { TN("settings-enum", "Command succeeds in terminal"), AlertSuccess },
    { TN("settings-enum", "Command fails in terminal"), AlertFailure },
    { TN("settings-enum", "String seen in terminal"), AlertString },
    { TN("settings-enum", "Regex seen in terminal"), AlertRegex },
    { TN("settings-enum", "Bell seen in terminal"), AlertBell },
    { TN("settings-enum", "Attribute is string"), AlertAString },
    { TN("settings-enum", "Attribute matches regex"), AlertARegex },
    { NULL }
};

static const SettingDef s_alertDefs[] = {
    { "Condition/Type", "condition", QVariant::Int,
      TN("settings-category", "Condition"),
      TN("settings", "Condition"),
      new ChoiceWidgetFactory(s_condArg)
    },
    { "Condition/InactivityTime", "idleTime", QVariant::Int,
      TN("settings-category", "Condition"),
      TN("settings", "Inactivity period"),
      new IntWidgetFactory(IntWidget::Millis, 0, 864000000, 1000)
    },
    { "Condition/SearchString", "condString", QVariant::String,
      TN("settings-category", "Condition"),
      TN("settings", "Match string or ECMAScript regular expression"),
      new InputWidgetFactory
    },
    { "Condition/AttributeName", "condAttr", QVariant::String,
      TN("settings-category", "Condition"),
      TN("settings", "Match attribute name"),
      new InputWidgetFactory
    },
    { "Actions/SwitchProfile", "actSwitch", QVariant::Bool,
      TN("settings-category", "Actions"),
      TN("settings", "Switch profile"),
      new CheckWidgetFactory
    },
    { "Actions/PushProfile", "actPush", QVariant::Bool,
      TN("settings-category", "Actions"),
      TN("settings", "Push profile"),
      new CheckWidgetFactory
    },
    { "Actions/InvokeAction", "actSlot", QVariant::Bool,
      TN("settings-category", "Actions"),
      TN("settings", "Invoke an application action"),
      new CheckWidgetFactory
    },
    { "Actions/RunLauncher", "actLaunch", QVariant::Bool,
      TN("settings-category", "Actions"),
      TN("settings", "Run a launcher"),
      new CheckWidgetFactory
    },
    { "Actions/DesktopNotify", "actNotify", QVariant::Bool,
      TN("settings-category", "Actions"),
      TN("settings", "Send a desktop notification message"),
      new CheckWidgetFactory
    },
    { "Actions/ShowIndicator", "actInd", QVariant::Bool,
      TN("settings-category", "Actions"),
      TN("settings", "Show an indicator in the terminal thumbnail"),
      new CheckWidgetFactory
    },
    { "Actions/FlashThumbnail", "actFlash", QVariant::Bool,
      TN("settings-category", "Actions"),
      TN("settings", "Flash effect in terminal thumbnail"),
      new CheckWidgetFactory
    },
    { "Actions/MoveTerminalForward", "actTerm", QVariant::Bool,
      TN("settings-category", "Actions"),
      TN("settings", "Move the terminal to the front of the list"),
      new CheckWidgetFactory
    },
    { "Actions/MoveServerForward", "actServer", QVariant::Bool,
      TN("settings-category", "Actions"),
      TN("settings", "Move the server to the front of the list"),
      new CheckWidgetFactory
    },
    { "Parameters/Profile", "paramProfile", QVariant::String,
      TN("settings-category", "Parameters"),
      TN("settings", "Profile to push or switch to"),
      new ProfileSelectFactory(false)
    },
    { "Parameters/Action", "paramSlot", QVariant::String,
      TN("settings-category", "Parameters"),
      TN("settings", "Application action to invoke"),
      new SlotWidgetFactory(true)
    },
    { "Parameters/Launcher", "paramLauncher", QVariant::String,
      TN("settings-category", "Parameters"),
      TN("settings", "Launcher to run"),
      new LauncherSelectFactory
    },
    { "Parameters/Flashes", "paramFlashes", QVariant::Int,
      TN("settings-category", "Parameters"),
      TN("settings", "Number of times to flash terminal thumbnail"),
      new IntWidgetFactory(IntWidget::Blinks, 0, 32767)
    },
    { "Parameters/Message", "paramMessage", QVariant::String,
      TN("settings-category", "Parameters"),
      TN("settings", "Desktop notification message"),
      new InputWidgetFactory
    },
    { NULL }
};

static SettingsBase::SettingsDef s_alertDef = {
    SettingsBase::Alert, s_alertDefs
};

void
AlertSettings::populateDefaults()
{
    auto &v = s_alertDef.defaults;
    v.insert(B("condition"), AlertActive);
    v.insert(B("actNotify"), true);
    v.insert(B("actInd"), true);
    v.insert(B("paramFlashes"), 5);
}

AlertSettings::AlertSettings(const QString &name, const QString &path) :
    SettingsBase(s_alertDef, path)
{
    m_name = name;
    m_needs = NeedsActivity;

    // initDefaults
    m_condition = AlertActive;
    m_idleTime = 0;
    m_paramFlashes = 5;
    m_actSwitch = false;
    m_actPush = false;
    m_actSlot = false;
    m_actLaunch = false;
    m_actNotify = true;
    m_actInd = true;
    m_actFlash = false;
    m_actTerm = false;
    m_actServer = false;
}

static inline void
reportUpdate(int row)
{
    if (row != -1)
        emit g_settings->alertUpdated(row);
}

/*
 * Non-properties
 */
QString
AlertSettings::condStr() const
{
    QString text = TL("settings-enum", s_condArg[m_condition].description);
    switch (m_condition) {
    case AlertIdle:
        text += L(": %1ms").arg(m_idleTime);
        break;
    case AlertString:
        text += A(": ") + m_condString;
        break;
    case AlertAString:
    case AlertARegex:
        text += A(": ") + m_condAttr;
        break;
    }
    return text;
}

QString
AlertSettings::actionStr(const char *property) const
{
    const SettingDef *s;
    for (s = s_alertDefs; strcmp(s->property, property); ++s);
    QString text = TL("settings", s->description);

    if (!strcmp(property, "actPush") || !strcmp(property, "actSwitch"))
        text += A(": ") + m_paramProfile;
    else if (!strcmp(property, "actSlot"))
        text += A(": ") + m_paramSlot.leftRef(m_paramSlot.indexOf('|'));
    else if (!strcmp(property, "actLaunch"))
        text += A(": ") + m_paramLauncher;

    return text;
}

QString
AlertSettings::slotStr(TermInstance *term) const
{
    QString tmp = m_paramSlot;
    tmp.replace(A("<terminalId>"), term->idStr());
    tmp.replace(A("<serverId>"), term->server()->idStr());
    return tmp;
}

ColorName
AlertSettings::activeColor() const
{
    return (m_refcount > 1) ? ConnFg : DisconnFg;
}

void
AlertSettings::setFavorite(unsigned favorite)
{
    m_favorite = favorite;
    reportUpdate(m_row);
}

void
AlertSettings::takeReference()
{
    ++m_refcount;
    reportUpdate(m_row);
}

void
AlertSettings::putReference()
{
    if (--m_refcount == 0)
       delete this;
    else
       reportUpdate(m_row);
}

void
AlertSettings::reportActivity(TermInstance *term)
{
    if (m_condition == AlertActive) {
        term->reportAlert();
    } else {
        auto i = m_timers.find(term);
        killTimer(*i);
        *i = startTimer(m_idleTime);
    }
}

void
AlertSettings::reportJob(TermInstance *term, const TermJob *job) const
{
    int code = job->attributes.value(g_attr_REGION_EXITCODE).toInt();

    switch (m_condition) {
    default:
        term->reportAlert();
        break;
    case AlertSuccess:
        if (code == 0)
            term->reportAlert();
        break;
    case AlertFailure:
        if (code != 0)
            term->reportAlert();
        break;
    }
}

void
AlertSettings::reportContent(TermInstance *term, const std::string &str) const
{
    if (m_condition == AlertString) {
        if (str.find(m_string) != std::string::npos)
            term->reportAlert();
    } else {
        if (m_regex.match(QString::fromStdString(str)).hasMatch())
            term->reportAlert();
    }
}

void
AlertSettings::reportBell(TermInstance *term) const
{
    term->reportAlert();
}

void
AlertSettings::reportAttribute(TermInstance *term, const QString &key,
                               const QString &value) const
{
    if (m_condAttr == key) {
        if (m_condition == AlertAString) {
            if (value.contains(m_condString))
                term->reportAlert();
        } else {
            if (m_regex.match(value).hasMatch())
                term->reportAlert();
        }
    }
}

void
AlertSettings::watch(TermInstance *term)
{
    switch (m_condition) {
    case AlertIdle:
        m_timers[term] = startTimer(m_idleTime);
        break;
    case AlertAString:
    case AlertARegex:
        auto i = term->attributes().constFind(m_condAttr);
        if (i != term->attributes().cend())
            reportAttribute(term, i.key(), *i);
        break;
    }
}

void
AlertSettings::unwatch(TermInstance *term)
{
    auto i = m_timers.constFind(term);
    if (i != m_timers.cend()) {
        killTimer(*i);
        m_timers.erase(i);
    }
}

void
AlertSettings::timerEvent(QTimerEvent *event)
{
    for (auto i = m_timers.cbegin(), j = m_timers.cend(); i != j; ++i)
        if (*i == event->timerId()) {
            i.key()->reportAlert();
            break;
        }
}

/*
 * Properties
 */
void
AlertSettings::setCondition(int condition)
{
    if (condition < 0 || condition >= AlertNTypes) {
        condition = 0;
    }
    if (m_condition != condition) {
        switch (m_condition = condition) {
        default:
            m_needs = NeedsActivity;
            break;
        case AlertFinish:
        case AlertSuccess:
        case AlertFailure:
            m_needs = NeedsJobs;
            break;
        case AlertString:
        case AlertRegex:
            m_needs = NeedsContent;
            break;
        case AlertBell:
            m_needs = NeedsBell;
            break;
        case AlertAString:
        case AlertARegex:
            m_needs = NeedsAttr;
            break;
        }

        emit settingChanged("condition", condition);
        reportUpdate(m_row);
    }
}

void
AlertSettings::setCondString(const QString &condString)
{
    if (m_condString != condString) {
        m_condString = condString;
        m_string = condString.toStdString();
        m_regex.setPattern(condString);
        emit settingChanged("condString", condString);
    }
}

REPORT_SETTER(AlertSettings::setCondAttr, condAttr, const QString &)
REPORT_SETTER(AlertSettings::setParamProfile, paramProfile, const QString &)
REPORT_SETTER(AlertSettings::setParamSlot, paramSlot, const QString &)
REPORT_SETTER(AlertSettings::setParamLauncher, paramLauncher, const QString &)
REG_SETTER(AlertSettings::setParamMessage, paramMessage, const QString &)
REG_SETTER(AlertSettings::setParamFlashes, paramFlashes, int)
REPORT_SETTER(AlertSettings::setIdleTime, idleTime, int)
REPORT_SETTER(AlertSettings::setActSwitch, actSwitch, bool)
REPORT_SETTER(AlertSettings::setActPush, actPush, bool)
REPORT_SETTER(AlertSettings::setActSlot, actSlot, bool)
REPORT_SETTER(AlertSettings::setActLaunch, actLaunch, bool)
REPORT_SETTER(AlertSettings::setActNotify, actNotify, bool)
REPORT_SETTER(AlertSettings::setActInd, actInd, bool)
REPORT_SETTER(AlertSettings::setActFlash, actFlash, bool)
REPORT_SETTER(AlertSettings::setActTerm, actTerm, bool)
REPORT_SETTER(AlertSettings::setActServer, actServer, bool)
