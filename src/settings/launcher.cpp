// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/icons.h"
#include "launcher.h"
#include "basemacros.h"
#include "settings.h"
#include "choicewidget.h"
#include "checkwidget.h"
#include "intwidget.h"
#include "inputwidget.h"
#include "commandwidget.h"
#include "envwidget.h"
#include "profileselect.h"
#include "imagewidget.h"
#include "downloadwidget.h"
#include "lib/enums.h"

#include <QRegularExpression>

static const QRegularExpression s_optsub(L("%\\?(\\w)\\{(.*?)\\}"));
static const QRegularExpression s_regsub(L("%\\w"));
static const QRegularExpression s_wantsFile(L("%(?:f|F|d|n)"));
static const QRegularExpression s_wantsUrl(L("%(?:u|U)"));

// Note: Must be sorted in ascending order
static const ChoiceDef s_launchArg[] = {
    { TN("settings-enum", "Open file via local desktop environment"),
      LaunchDefault, ICON_LAUNCHTYPE_DEFAULT },
    { TN("settings-enum", "Run bare command on the local machine"),
      LaunchLocalCommand, ICON_LAUNCHTYPE_RUN_LOCAL },
    { TN("settings-enum", "Run bare command on the remote machine"),
      LaunchRemoteCommand, ICON_LAUNCHTYPE_RUN_REMOTE },
    { TN("settings-enum", "Run command in a new local terminal"),
      LaunchLocalTerm, ICON_LAUNCHTYPE_TERM_LOCAL },
    { TN("settings-enum", "Run command in a new remote terminal"),
      LaunchRemoteTerm, ICON_LAUNCHTYPE_TERM_REMOTE },
    { TN("settings-enum", "Write command text into the active terminal"),
      LaunchWriteCommand, ICON_LAUNCHTYPE_WRITE },
    { NULL }
};

static const ChoiceDef s_mountArg[] = {
    { TN("settings-enum", "Mount read-write"), MountReadWrite },
    { TN("settings-enum", "Mount read-only"), MountReadOnly },
    { TN("settings-enum", "Do not mount (local files only)"), MountNone },
    { NULL }
};

static const ChoiceDef s_outputArg[] = {
    { TN("settings-enum", "Discard output"), InOutNone },
    { TN("settings-enum", "Write output to a local file"), InOutFile },
    { TN("settings-enum", "Display output in a dialog box"), InOutDialog },
    { TN("settings-enum", "Copy output to the clipboard"), InOutCopy },
    { TN("settings-enum", "Write output to the active terminal"), InOutWrite },
    { TN("settings-enum", "Interpret output as an action to run"), InOutAction },
    { NULL }
};

static const ChoiceDef s_inputArg[] = {
    { TN("settings-enum", "No input"), InOutNone },
    { TN("settings-enum", "Read input from a local file"), InOutFile },
    { NULL }
};

static const ChoiceDef s_outputConfig[] = {
    { TN("settings-enum", "Ask what to do"), Tsq::TaskAsk },
    { TN("settings-enum", "Overwrite without asking"), Tsq::TaskOverwrite },
    { TN("settings-enum", "Rename without asking"), Tsq::TaskRename },
    { TN("settings-enum", "Fail"), Tsq::TaskFail },
    { NULL }
};

static const SettingDef s_launchDefs[] = {
    { "Match/FileExtensions", "extensions", QVariant::String,
      TN("settings-category", "Match"),
      TN("settings", "Optional space-separated list of filename extensions"),
      new InputWidgetFactory
    },
    { "Match/URISchemes", "schemes", QVariant::String,
      TN("settings-category", "Match"),
      TN("settings", "Optional space-separated list of URI schemes"),
      new InputWidgetFactory
    },
    { "Launcher/LaunchType", "launchType", QVariant::Int,
      TN("settings-category", "Launcher"),
      TN("settings", "This launcher will"),
      new ChoiceWidgetFactory(s_launchArg)
    },
    { "Launcher/Profile", "profile", QVariant::String,
      TN("settings-category", "Launcher"),
      TN("settings", "Profile to use if creating a terminal"),
      new ProfileSelectFactory(true)
    },
    { "Launcher/MountType", "mountType", QVariant::Int,
      TN("settings-category", "Launcher"),
      TN("settings", "Type of mount to perform if file is remote"),
      new ChoiceWidgetFactory(s_mountArg)
    },
    { "Launcher/UnmountIdleTime", "mountIdle", QVariant::Int,
      TN("settings-category", "Launcher"),
      TN("settings", "Unmount file automatically after idle period"),
      new IntWidgetFactory(IntWidget::Minutes, 0, 10000, 1, true)
    },
    { "Launcher/DisplayIcon", "icon", QVariant::String,
      TN("settings-category", "Launcher"),
      TN("settings", "Display icon"),
      new ImageWidgetFactory(ThumbIcon::CommandType)
    },
    { "Command/Command", "command", QVariant::StringList,
      TN("settings-category", "Command"),
      TN("settings", "Command using %f,%n,%d markers for path file dir"),
      new CommandWidgetFactory
    },
    { "Command/Directory", "directory", QVariant::String,
      TN("settings-category", "Command"),
      TN("settings", "Directory"),
      new InputWidgetFactory
    },
    { "Command/FileDirectory", "fileDirectory", QVariant::Bool,
      TN("settings-category", "Command"),
      TN("settings", "Start in same directory as file being launched"),
      new CheckWidgetFactory
    },
    { "Command/Environment", "environ", QVariant::StringList,
      TN("settings-category", "Command"),
      TN("settings", "Environment"),
      new EnvironWidgetFactory(false)
    },
    { "InputOutput/InputType", "inputType", QVariant::Int,
      TN("settings-category", "Input/Output"),
      TN("settings", "Input stream to bare command"),
      new ChoiceWidgetFactory(s_inputArg)
    },
    { "InputOutput/InputFile", "inputFile", QVariant::String,
      TN("settings-category", "Input/Output"),
      TN("settings", "Input file"),
      new DownloadWidgetFactory(2)
    },
    { "InputOutput/OutputType", "outputType", QVariant::Int,
      TN("settings-category", "Input/Output"),
      TN("settings", "Output stream of bare command"),
      new ChoiceWidgetFactory(s_outputArg)
    },
    { "InputOutput/OutputFile", "outputFile", QVariant::String,
      TN("settings-category", "Input/Output"),
      TN("settings", "Output file"),
      new DownloadWidgetFactory(3)
    },
    { "InputOutput/OutputFileConfirmation", "outputConfig", QVariant::Int,
      TN("settings-category", "Input/Output"),
      TN("settings", "If output file already exists"),
      new ChoiceWidgetFactory(s_outputConfig)
    },
    { NULL }
};

static SettingsBase::SettingsDef s_launchDef = {
    SettingsBase::Launch, s_launchDefs
};

void
LaunchSettings::populateDefaults()
{
    auto &v = s_launchDef.defaults;
    v.insert(B("mountType"), MountReadWrite);
    v.insert(B("fileDirectory"), true);
    v.insert(B("inputFile"), g_str_PROMPT_PROFILE);
    v.insert(B("outputFile"), g_str_PROMPT_PROFILE);
    v.insert(B("outputConfig"), Tsq::TaskAsk);
}

LaunchSettings::LaunchSettings(const QString &name, const QString &path, bool config) :
    SettingsBase(s_launchDef, path, config),
    m_config(config)
{
    m_name = name;
    m_reserved = !config || name == g_str_DEFAULT_PROFILE;

    // initDefaults
    m_launchType = 0;
    m_mountType = MountReadWrite;
    m_mountIdle = 0;
    m_inputType = InOutNone;
    m_outputType = InOutNone;
    m_outputConfig = Tsq::TaskAsk;
    m_fileDirectory = true;
}

static inline void
reportUpdate(int row)
{
    if (row != -1)
        emit g_settings->launcherUpdated(row);
}

/*
 * Non-properties
 */
void
LaunchSettings::setFavorite(unsigned favorite)
{
    m_favorite = favorite;
    reportUpdate(m_row);
}

void
LaunchSettings::setDefault(bool isdefault)
{
    m_default = isdefault;
    reportUpdate(m_row);
}

bool
LaunchSettings::matches(const QUrl &url) const
{
    if (url.isLocalFile()) {
        if (!m_wantsFile)
            return false;

        QString filename = url.fileName();

        for (const auto &str: m_extList)
            if (filename.endsWith(str))
                return true;

        return m_extList.isEmpty();
    }
    else {
        return m_wantsUrl ?
            m_schList.isEmpty() || m_schList.contains(url.scheme()) :
            false;
    }
}

LaunchParams
LaunchSettings::getParams(const AttributeMap &subs, char sep) const
{
    // First do conditional strings
    QStringList tmp = m_command;
    QRegularExpressionMatch match;
    QString name('%'), repl;

    for (auto i = tmp.begin(); i != tmp.end(); ) {
        if (i->isEmpty()) {
            ++i;
            continue;
        }
        while ((match = s_optsub.match(*i)).hasMatch()) {
            if (subs.contains(name + match.captured(1)))
                repl = match.captured(2);

            i->replace(match.capturedStart(), match.capturedLength(), repl);
        }
        if (i->isEmpty())
            i = tmp.erase(i);
        else
            ++i;
    }

    // Next perform replacements
    LaunchParams lp = { tmp.join(sep) };
    int pos = 0;
    while ((match = s_regsub.match(lp.cmd, pos)).hasMatch()) {
        auto i = subs.constFind(match.captured());
        if (i == subs.cend()) {
            pos = match.capturedEnd();
        } else {
            pos = match.capturedStart();
            lp.cmd.replace(pos, match.capturedLength(), *i);
            pos += i->size();
        }
    }

    if (m_fileDirectory) {
        auto i = subs.find(A("%d"));
        if (i != subs.end())
            lp.dir = *i;
    }

    lp.env = m_environ;
    return lp;
}

QString
LaunchSettings::getText(const AttributeMap &subs) const
{
    QStringList list = getParams(subs, '\0').cmd.split('\0');
    if (list.size() > 1)
        list.removeAt(1);
    return list.join(' ');
}

QString
LaunchSettings::makeCommandStr(QStringList list)
{
    if (list.size() > 1) {
        if (list.at(0) != list.at(1)) {
            list[0] = L("%1[%2]").arg(list.at(0), list.at(1));
        }
        list.removeAt(1);
    }

    return list.join(' ');
}

/*
 * Properties
 */
void
LaunchSettings::setCommand(const QStringList &command)
{
    if (m_command != command) {
        if (command.isEmpty() || command.at(0).isEmpty()) {
            m_command.clear();
            m_commandStr.clear();
            m_wantsFile = m_wantsUrl = false;
        } else {
            m_command = command;
            m_commandStr = makeCommandStr(command);
            m_wantsUrl = s_wantsUrl.match(m_commandStr).hasMatch();
            m_wantsFile = m_wantsUrl || s_wantsFile.match(m_commandStr).hasMatch();
        }
        emit settingChanged("command", m_command);
        reportUpdate(m_row);
    }
}

void
LaunchSettings::setExtensions(const QString &extensions)
{
    if (m_extensions != extensions) {
        m_extensions = extensions;

        m_extList = extensions.split(' ', QString::SkipEmptyParts);
        for (auto &str: m_extList)
            if (str.startsWith(A("*.")))
                str.remove(0, 1);
            else if (!str.startsWith('.'))
                str.prepend('.');

        m_patternStr = m_extList.mid(0, 3).join(' ');
        if (m_extList.size() > 3)
            m_patternStr += L(" +%1").arg(m_extList.size() - 3);

        emit settingChanged("extensions", extensions);
        reportUpdate(m_row);
    }
}

void
LaunchSettings::setSchemes(const QString &schemes)
{
    if (m_schemes != schemes) {
        m_schemes = schemes;
        m_schList = schemes.split(' ', QString::SkipEmptyParts);
        emit settingChanged("schemes", schemes);
    }
}

void
LaunchSettings::setIcon(const QString &icon)
{
    if (m_icon != icon) {
        m_icon = icon;

        m_nameIcon = !icon.isEmpty() ?
            ThumbIcon::getIcon(ThumbIcon::CommandType, icon) :
            QIcon();

        emit settingChanged("icon", icon);
        reportUpdate(m_row);
    }
}

void
LaunchSettings::setLaunchType(int launchType)
{
    if (launchType < 0 || launchType >= LaunchNTypes)
        launchType = 0;

    if (m_launchType != launchType) {
        m_launchType = launchType;

        m_typeIcon = ThumbIcon::fromTheme(s_launchArg[launchType].icon);

        emit settingChanged("launchType", launchType);
        reportUpdate(m_row);
    }
}

void
LaunchSettings::setMountType(int mountType)
{
    if (mountType < 0 || mountType >= MountNTypes)
        mountType = 0;

    if (m_mountType != mountType) {
        m_mountType = mountType;
        emit settingChanged("mountType", mountType);
        reportUpdate(m_row);
    }
}

REG_SETTER(LaunchSettings::setEnviron, environ, const QStringList &)
REG_SETTER(LaunchSettings::setDirectory, directory, const QString &)
REPORT_SETTER(LaunchSettings::setProfile, profile, const QString &)
REG_SETTER(LaunchSettings::setInputFile, inputFile, const QString &)
REG_SETTER(LaunchSettings::setOutputFile, outputFile, const QString &)
REG_SETTER(LaunchSettings::setMountIdle, mountIdle, int)
REG_SETTER(LaunchSettings::setInputType, inputType, int)
REG_SETTER(LaunchSettings::setOutputType, outputType, int)
REG_SETTER(LaunchSettings::setOutputConfig, outputConfig, int)
REG_SETTER(LaunchSettings::setFileDirectory, fileDirectory, bool)
