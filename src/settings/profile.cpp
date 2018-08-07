// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/enums.h"
#include "app/format.h"
#include "profile.h"
#include "basemacros.h"
#include "keymap.h"
#include "settings.h"
#include "checkwidget.h"
#include "keymapselect.h"
#include "fontselect.h"
#include "colorselect.h"
#include "sizewidget.h"
#include "layoutwidget.h"
#include "choicewidget.h"
#include "commandwidget.h"
#include "inputwidget.h"
#include "formatwidget.h"
#include "powerwidget.h"
#include "envwidget.h"
#include "palettewidget.h"
#include "dircolorswidget.h"
#include "intwidget.h"
#include "imagewidget.h"
#include "encodingselect.h"
#include "encodingcheck.h"
#include "langwidget.h"
#include "lib/unicode.h"

static const ChoiceDef s_exitActionArg[] = {
    { TN("settings-enum", "Stop the emulator"), Tsq::ExitActionStop },
    { TN("settings-enum", "Restart the process"), Tsq::ExitActionRestart },
    { TN("settings-enum", "Clear scrollback and restart the process"), Tsq::ExitActionClear },
    { NULL }
};

static const ChoiceDef s_autoCloseArg[] = {
    { TN("settings-enum", "Always"), Tsq::AutoCloseAlways },
    { TN("settings-enum", "Only if child process exits normally"), Tsq::AutoCloseExit },
    { TN("settings-enum", "Only if child process exits with status 0"), Tsq::AutoCloseZero },
    { TN("settings-enum", "Never"), Tsq::AutoCloseNever },
    { NULL }
};

static const ChoiceDef s_promptCloseArg[] = {
    { TN("settings-enum", "Always"), Tsqt::PromptCloseAlways },
    { TN("settings-enum", "If child process is running"), Tsqt::PromptCloseProc },
    { TN("settings-enum", "If foreground job is running"), Tsqt::PromptCloseSubproc },
    { TN("settings-enum", "Never"), Tsqt::PromptCloseNever },
    { NULL }
};

static const ChoiceDef s_remoteSettingArg[] = {
    { TN("settings-enum", "Ignore remote settings"), ChProf },
    { TN("settings-enum", "Show remote user settings"), ChPref },
    { TN("settings-enum", "Show remote user and program settings"), ChSess },
    { NULL }
};

static const ChoiceDef s_fileStyleArg[] = {
    { TN("settings-enum", "Long listing (ls -l)"), FilesLong },
    { TN("settings-enum", "Short listing (ls)"), FilesShort },
    { NULL }
};

static const ChoiceDef s_fileEffectArg[] = {
    { TN("settings-enum", "Always"), WindowAlways },
    { TN("settings-enum", "When window has focus"), WindowFocus },
    { TN("settings-enum", "Never"), WindowNever },
    { NULL }
};

static const ChoiceDef s_exitEffectArg[] = {
    { TN("settings-enum", "Always"), WindowAlways },
    { TN("settings-enum", "When terminal is not focused"), WindowFocus },
    { TN("settings-enum", "Never"), WindowNever },
    { NULL }
};

static const ChoiceDef s_unicodeVariant[] = {
    { TSQ_UNICODE_VARIANT_100, TSQ_UNICODE_VARIANT_100 },
    { NULL }
};

static const SettingDef s_profileDefs[] = {
    { "Input/Keymap", "keymapName", QVariant::String,
      TN("settings-category", "Input"),
      TN("settings", "Keymap"),
      new KeymapSelectFactory
    },
    { "Input/SendMouseEvents", "mouseMode", QVariant::Bool,
      TN("settings-category", "Input"),
      TN("settings", "Honor mouse tracking modes"),
      new CheckWidgetFactory
    },
    { "Input/SendFocusEvents", "focusMode", QVariant::Bool,
      TN("settings-category", "Input"),
      TN("settings", "Honor focus tracking mode"),
      new CheckWidgetFactory
    },
    { "Input/SendScrollEvents", "scrollMode", QVariant::Bool,
      TN("settings-category", "Input"),
      TN("settings", "Honor alternate scroll mode"),
      new CheckWidgetFactory
    },
    { TSQ_SETTING_FLAGS, "emulatorFlags", QVariant::ULongLong,
      TN("settings-category", "Input"),
      TN("settings", "Start in alternate scroll mode"),
      new CheckWidgetFactory(Tsq::AltScrollMouseMode)
    },
    //
    // The following entries need to stay in order
    //
    { "Appearance/BackgroundColor", NULL, QVariant::Invalid,
      TN("settings-category", "Appearance"),
      TN("settings", "Background color"),
      new ColorSelectFactory(261)
    },
    { "Appearance/ForegroundColor", NULL, QVariant::Invalid,
      TN("settings-category", "Appearance"),
      TN("settings", "Foreground color"),
      new ColorSelectFactory(260)
    },
    { "Appearance/Palette", "palette", QVariant::String,
      TN("settings-category", "Appearance"),
      TN("settings", "Color theme"),
      new PaletteWidgetFactory
    },
    //
    // End ordering requirement
    //
    { "Appearance/Font", "font", QVariant::String,
      TN("settings-category", "Appearance"),
      TN("settings", "Font"),
      new FontSelectFactory(true)
    },
    { "Appearance/WidgetLayout", "layout", QVariant::String,
      TN("settings-category", "Appearance"),
      TN("settings", "Layout scrollbar, minimap, and marks widgets"),
      new LayoutWidgetFactory(0)
    },
    { "Appearance/ColumnFills", "fills", QVariant::String,
      TN("settings-category", "Appearance"),
      TN("settings", "Show fill lines at column positions"),
      new LayoutWidgetFactory(1)
    },
    { "Appearance/Badge", "badge", QVariant::String,
      TN("settings-category", "Appearance"),
      TN("settings", "Format of terminal badge text"),
      new FormatWidgetFactory(g_termFormat)
    },
    { "Appearance/ShowMainIndicators", "mainIndicator", QVariant::Bool,
      TN("settings-category", "Appearance"),
      TN("settings", "Show ownership and mode indicators in terminal"),
      new CheckWidgetFactory
    },
    { "Appearance/ShowThumbnailIndicators", "thumbIndicator", QVariant::Bool,
      TN("settings-category", "Appearance"),
      TN("settings", "Show ownership and mode indicators in thumbnail"),
      new CheckWidgetFactory
    },
    { "Appearance/ShowThumbnailIcon", "showIcon", QVariant::Bool,
      TN("settings-category", "Appearance"),
      TN("settings", "Show icon in thumbnail view"),
      new CheckWidgetFactory
    },
    { "Appearance/FixedThumbnailIcon", "icon", QVariant::String,
      TN("settings-category", "Appearance"),
      TN("settings", "Show specific icon in thumbnail view"),
      new ImageWidgetFactory(ThumbIcon::TerminalType)
    },
    { "Appearance/NumRecentPrompts", "recentPrompts", QVariant::Int,
      TN("settings-category", "Appearance"),
      TN("settings", "Number of recent prompts to show in minimap"),
      new IntWidgetFactory(IntWidget::Prompts, 0, MAX_MINIMAP_PROMPTS)
    },
    { "Appearance/ShowFetchPosition", "fetchPos", QVariant::Bool,
      TN("settings-category", "Appearance"),
      TN("settings", "Show scrollback download position in minimap "),
      new CheckWidgetFactory
    },
    { "Effects/ExitStatusEffect", "exitEffect", QVariant::Int,
      TN("settings-category", "Effects"),
      TN("settings", "Flash terminal thumbnail when a process exits"),
      new ChoiceWidgetFactory(s_exitEffectArg)
    },
    { "Effects/ExitStatusRuntime", "exitRuntime", QVariant::Int,
      TN("settings-category", "Effects"),
      TN("settings", "Minimum process runtime to show flash effect"),
      new IntWidgetFactory(IntWidget::Millis, 0, 86400000, 250)
    },
    { "Collaboration/AllowRemoteInput", "remoteInput", QVariant::Bool,
      TN("settings-category", "Collaboration"),
      TN("settings", "Allow other users to send input to my terminals"),
      new CheckWidgetFactory
    },
    { "Collaboration/ShowRemoteFont", "remoteFont", QVariant::Int,
      TN("settings-category", "Collaboration"),
      TN("settings", "Show fonts set by remote users or programs"),
      new ChoiceWidgetFactory(s_remoteSettingArg)
    },
    { "Collaboration/ShowRemoteColors", "remoteColors", QVariant::Int,
      TN("settings-category", "Collaboration"),
      TN("settings", "Show colors set by remote users or programs"),
      new ChoiceWidgetFactory(s_remoteSettingArg)
    },
    { "Collaboration/ShowRemoteLayout", "remoteLayout", QVariant::Int,
      TN("settings-category", "Collaboration"),
      TN("settings", "Show layout set by remote users or programs"),
      new ChoiceWidgetFactory(s_remoteSettingArg)
    },
    { "Collaboration/ShowRemoteFills", "remoteFills", QVariant::Int,
      TN("settings-category", "Collaboration"),
      TN("settings", "Show column fills set by remote users or programs"),
      new ChoiceWidgetFactory(s_remoteSettingArg)
    },
    { "Collaboration/ShowRemoteBadge", "remoteBadge", QVariant::Int,
      TN("settings-category", "Collaboration"),
      TN("settings", "Show badge set by remote users or programs"),
      new ChoiceWidgetFactory(s_remoteSettingArg)
    },
    { "Collaboration/ResetRemoteOnTakingOwnership", "remoteReset", QVariant::Bool,
      TN("settings-category", "Collaboration"),
      TN("settings", "Discard remote settings when taking terminal ownership"),
      new CheckWidgetFactory
    },
    { "Collaboration/FollowRemoteScrolling", "followDefault", QVariant::Bool,
      TN("settings-category", "Collaboration"),
      TN("settings", "Follow remote terminal scrolling by default"),
      new CheckWidgetFactory
    },
    { "Collaboration/AllowRemoteClipboard", "remoteClipboard", QVariant::Bool,
      TN("settings-category", "Collaboration"),
      TN("settings", "Allow terminal programs to write to the clipboard"),
      new CheckWidgetFactory
    },
    { "Emulator/TermSize", "termSize", QVariant::Size,
      TN("settings-category", "Emulator"),
      TN("settings", "Preferred terminal size"),
      new SizeWidgetFactory
    },
    { TSQ_SETTING_CAPORDER, "scrollbackSize", QVariant::UInt,
      TN("settings-category", "Emulator"),
      TN("settings", "Scrollback buffer size as power of 2"),
      new PowerWidgetFactory(TERM_MAX_CAPORDER)
    },
    { TSQ_SETTING_COMMAND, "command", QVariant::StringList,
      TN("settings-category", "Emulator"),
      TN("settings", "Command"),
      new CommandWidgetFactory
    },
    { TSQ_SETTING_STARTDIR, "directory", QVariant::String,
      TN("settings-category", "Emulator"),
      TN("settings", "Directory"),
      new InputWidgetFactory
    },
    { "Emulator/StartInSameDirectory", "sameDirectory", QVariant::Bool,
      TN("settings-category", "Emulator"),
      TN("settings", "Use same directory as current terminal"),
      new CheckWidgetFactory
    },
    { TSQ_SETTING_ENVIRON, "environ", QVariant::StringList,
      TN("settings-category", "Emulator"),
      TN("settings", "Environment"),
      new EnvironWidgetFactory(true)
    },
    { TSQ_SETTING_EXITACTION, "exitAction", QVariant::Int,
      TN("settings-category", "Emulator"),
      TN("settings", "Action when process terminates"),
      new ChoiceWidgetFactory(s_exitActionArg)
    },
    { TSQ_SETTING_AUTOCLOSE, "autoClose", QVariant::Int,
      TN("settings-category", "Emulator"),
      TN("settings", "Auto-close terminal when emulator stops"),
      new ChoiceWidgetFactory(s_autoCloseArg)
    },
    { TSQ_SETTING_AUTOCLOSETIME, "autoCloseTime", QVariant::Int,
      TN("settings-category", "Emulator"),
      TN("settings", "Minimum runtime required before auto-close"),
      new IntWidgetFactory(IntWidget::Millis, 0, 60000, 500)
    },
    { "Emulator/PromptClose", "promptClose", QVariant::Int,
      TN("settings-category", "Emulator"),
      TN("settings", "Prompt before closing terminal"),
      new ChoiceWidgetFactory(s_promptCloseArg)
    },
    { TSQ_SETTING_PROMPTNEWLINE, "promptNewline", QVariant::Bool,
      TN("settings-category", "Emulator"),
      TN("settings", "Force newline before command prompts"),
      new CheckWidgetFactory
    },
    { TSQ_SETTING_SCROLLCLEAR, "scrollClear", QVariant::Bool,
      TN("settings-category", "Emulator"),
      TN("settings", "Clear screen by scrolling when prompt is active"),
      new CheckWidgetFactory
    },
    { TSQ_SETTING_MESSAGE, "message", QVariant::String,
      TN("settings-category", "Emulator"),
      TN("settings", "Print this message on startup"),
      new InputWidgetFactory
    },
    //
    // The following entries need to stay in order
    //
    { TSQ_SETTING_LANG, "lang", QVariant::String,
      TN("settings-category", "Encoding"),
      TN("settings", "Language"),
      new LangWidgetFactory
    },
    { "Encoding/SetLangEnvironmentVariable", "langEnv", QVariant::Bool,
      TN("settings-category", "Encoding"),
      TN("settings", "Set locale environment variables in terminal"),
      new CheckWidgetFactory
    },
    { TSQ_SETTING_ENCODING, "unicoding", QVariant::StringList,
      TN("settings-category", "Encoding"),
      TN("settings", "Unicode variant"),
      new EncodingSelectFactory(s_unicodeVariant)
    },
    { "Encoding/UseEmoji", NULL, QVariant::Invalid,
      TN("settings-category", "Encoding"),
      TN("settings", "Display emoji"),
      new EncodingCheckFactory(TSQ_UNICODE_PARAM_EMOJI)
    },
    { "Encoding/DoubleWidthAmbiguous", NULL, QVariant::Invalid,
      TN("settings-category", "Encoding"),
      TN("settings", "Treat ambiguous-width characters as double width"),
      new EncodingCheckFactory(TSQ_UNICODE_PARAM_WIDEAMBIG)
    },
    //
    // End ordering requirement
    //
    { "Encoding/WarnOnUnsupportedEncoding", "encodingWarn", QVariant::Bool,
      TN("settings-category", "Encoding"),
      TN("settings", "Warn if server uses an unsupported encoding"),
      new CheckWidgetFactory
    },
    { TSQ_SETTING_NFILES, "fileLimit", QVariant::UInt,
      TN("settings-category", "Files"),
      TN("settings", "Maximum directory size that can be monitored"),
      new IntWidgetFactory(IntWidget::Files, 0, 1000000)
    },
    { "Files/FileDisplayFormat", "fileStyle", QVariant::Int,
      TN("settings-category", "Files"),
      TN("settings", "Display format of files tool"),
      new ChoiceWidgetFactory(s_fileStyleArg)
    },
    { "Files/FileChangeEffect", "fileEffect", QVariant::Int,
      TN("settings-category", "Files"),
      TN("settings", "Enable animation on file change"),
      new ChoiceWidgetFactory(s_fileEffectArg)
    },
    { "Files/ShowHiddenFiles", "showDotfiles", QVariant::Bool,
      TN("settings-category", "Files"),
      TN("settings", "Show hidden files (ls -A)"),
      new CheckWidgetFactory
    },
    { "Files/FileClassify", "fileClassify", QVariant::Bool,
      TN("settings-category", "Files"),
      TN("settings", "Show file types (ls -F)"),
      new CheckWidgetFactory
    },
    { "Files/FileColorize", "fileColorize", QVariant::Bool,
      TN("settings-category", "Files"),
      TN("settings", "Show file colors (ls --color)"),
      new CheckWidgetFactory
    },
    { "Files/FileGittify", "fileGittify", QVariant::Bool,
      TN("settings-category", "Files"),
      TN("settings", "Show git status indicators in short format"),
      new CheckWidgetFactory
    },
    { "Files/ShowGitBanner", "fileGitline", QVariant::Bool,
      TN("settings-category", "Files"),
      TN("settings", "Display git information above directory name"),
      new CheckWidgetFactory
    },
    { "Files/Dircolors", "dircolors", QVariant::String,
      TN("settings-category", "Files"),
      TN("settings", "Dircolors database with git status support"),
      new DircolorsWidgetFactory
    },
    { "Files/SetLsColorsEnvironmentVariable", "dircolorsEnv", QVariant::Bool,
      TN("settings-category", "Files"),
      TN("settings", "Set dircolors environment variables in terminal"),
      new CheckWidgetFactory
    },
    { NULL }
};

static SettingsBase::SettingsDef s_profileDef = {
    SettingsBase::Profile, s_profileDefs
};

void
ProfileSettings::populateDefaults(const QString &defaultFont)
{
    auto &v = s_profileDef.defaults;
    v.insert(B("keymapName"), g_str_DEFAULT_KEYMAP);
    v.insert(B("mouseMode"), true);
    v.insert(B("focusMode"), true);
    v.insert(B("scrollMode"), true);
    v.insert(B("emulatorFlags"), (qulonglong)Tsq::AltScrollMouseMode);
    v.insert(B("font"), defaultFont);
    v.insert(B("layout"), TERM_LAYOUT);
    v.insert(B("badge"), DEFAULT_TERM_BADGE);
    v.insert(B("mainIndicator"), true);
    v.insert(B("thumbIndicator"), true);
    v.insert(B("showIcon"), true);
    v.insert(B("recentPrompts"), DEFAULT_MINIMAP_PROMPTS);
    v.insert(B("exitEffect"), WindowFocus);
    v.insert(B("exitRuntime"), EXIT_EFFECT_RUNTIME);
    v.insert(B("remoteInput"), true);
    v.insert(B("remoteFont"), 2);
    v.insert(B("remoteColors"), 2);
    v.insert(B("remoteLayout"), 2);
    v.insert(B("remoteFills"), 2);
    v.insert(B("remoteBadge"), 2);
    v.insert(B("remoteReset"), true);
    v.insert(B("remoteClipboard"), true);
    v.insert(B("termSize"), QSize(TERM_COLS, TERM_ROWS));
    v.insert(B("scrollbackSize"), TERM_DEF_CAPORDER);
    v.insert(B("command"), L(TERM_COMMAND).split('\x1f'));
    v.insert(B("sameDirectory"), true);
    v.insert(B("environ"), L(SESSION_ENVIRON).split('\x1f'));
    v.insert(B("exitAction"), Tsq::ExitActionStop);
    v.insert(B("autoClose"), Tsq::AutoCloseExit);
    v.insert(B("autoCloseTime"), DEFAULT_AUTOCLOSE_TIME);
    v.insert(B("promptClose"), Tsqt::PromptCloseSubproc);
    v.insert(B("promptNewline"), true);
    v.insert(B("scrollClear"), true);
    v.insert(B("langEnv"), true);
    v.insert(B("unicoding"), L(TSQ_UNICODE_DEFAULT).split('\x1f'));
    v.insert(B("fileLimit"), FILEMON_DEFAULT_LIMIT);
    v.insert(B("fileStyle"), FilesShort);
    v.insert(B("fileEffect"), WindowAlways);
    v.insert(B("fileClassify"), true);
    v.insert(B("fileColorize"), true);
    v.insert(B("fileGittify"), true);
    v.insert(B("fileGitline"), true);
    v.insert(B("dircolorsEnv"), true);
}

ProfileSettings::ProfileSettings(const QString &name, const QString &path) :
    SettingsBase(s_profileDef, path)
{
    m_name = name;
    m_reserved = (name == g_str_DEFAULT_PROFILE);
    m_refcount = 0;

    // initDefaults
    m_emulatorFlags = Tsq::AltScrollMouseMode;
    m_scrollbackSize = TERM_DEF_CAPORDER;
    m_fileLimit = FILEMON_DEFAULT_LIMIT;
    m_exitAction = Tsq::ExitActionStop;
    m_autoClose = Tsq::AutoCloseExit;
    m_autoCloseTime = DEFAULT_AUTOCLOSE_TIME;
    m_promptClose = Tsqt::PromptCloseSubproc;
    m_remoteFont = 2;
    m_remoteColors = 2;
    m_remoteLayout = 2;
    m_remoteFills = 2;
    m_remoteBadge = 2;
    m_recentPrompts = DEFAULT_MINIMAP_PROMPTS;
    m_fileStyle = FilesShort;
    m_fileEffect = WindowAlways;
    m_exitEffect = WindowFocus;
    m_exitRuntime = EXIT_EFFECT_RUNTIME;

    m_mainIndicator = true;
    m_thumbIndicator = true;
    m_showIcon = true;
    m_sameDirectory = true;
    m_langEnv = true;
    m_encodingWarn = false;
    m_promptNewline = true;
    m_scrollClear = true;
    m_followDefault = false;
    m_remoteInput = true;
    m_remoteClipboard = true;
    m_remoteReset = true;
    m_showDotfiles = false;
    m_fileClassify = true;
    m_fileGittify = true;
    m_fileColorize = true;
    m_fileGitline = true;
    m_dircolorsEnv = true;
    m_fetchPos = false;
    m_mouseMode = true;
    m_focusMode = true;
    m_scrollMode = true;
}

ProfileSettings::~ProfileSettings()
{
    if (m_keymap)
        m_keymap->putProfileReference();
}

static inline void
reportUpdate(int row)
{
    if (row != -1)
        emit g_settings->profileUpdated(row);
}

/*
 * Utilities
 */
void
ProfileSettings::toAttributes(AttributeMap &map) const
{
    QString prefix(g_attr_PROFILE_PREFIX);
    map[g_attr_PROFILE] = m_name;

    for (const SettingDef *s = s_profileDefs; s->key; ++s)
    {
        if (s->property)
            map[prefix + s->key] = s->factory->toString(property(s->property));
    }
}

std::pair<QString,QString>
ProfileSettings::toAttribute(const char *prop) const
{
    const SettingDef *s;
    for (s = s_profileDefs; !s->property || strcmp(s->property, prop); ++s);

    return std::make_pair(g_attr_PROFILE_PREFIX + s->key,
                          s->factory->toString(property(prop)));
}

void
ProfileSettings::fromAttributes(const AttributeMap &map)
{
    QString prefix(g_attr_PROFILE_PREFIX);

    for (const SettingDef *s = s_profileDefs; s->key; ++s)
    {
        if (!s->property)
            continue;

        auto i = map.constFind(prefix + s->key);
        if (i != map.cend()) {
            QVariant value = s->factory->fromString(*i);
            if (value.isValid())
                setProperty(s->property, value);
        }
    }
}

void
ProfileSettings::handleKeymapDestroyed()
{
    m_keymap = nullptr;

    if (!g_settings->closing())
        setKeymapName(g_str_DEFAULT_KEYMAP);
}

/*
 * Non-properties
 */
void
ProfileSettings::setFavorite(unsigned favorite)
{
    m_favorite = favorite;
    reportUpdate(m_row);
}

void
ProfileSettings::setDefault(bool isdefault)
{
    m_default = isdefault;
    reportUpdate(m_row);
}

void
ProfileSettings::takeReference()
{
    ++m_refcount;
    reportUpdate(m_row);
}

void
ProfileSettings::putReference()
{
    --m_refcount;
    reportUpdate(m_row);
}

/*
 * Properties
 */
void
ProfileSettings::setCommand(const QStringList &cmd)
{
    if (cmd.size() < 2 || cmd.at(0).isEmpty() || cmd.at(1).isEmpty()) {
        m_command = L(TERM_COMMAND).split('\x1f');
        emit settingChanged("command", m_command);
    } else if (m_command != cmd) {
        m_command = cmd;
        emit settingChanged("command", m_command);
    }
}

void
ProfileSettings::setUnicoding(const QStringList &unicoding)
{
    if (unicoding.isEmpty()) {
        m_unicoding = L(TSQ_UNICODE_DEFAULT).split('\x1f');
        emit settingChanged("unicoding", unicoding);
    } else if (m_unicoding != unicoding) {
        m_unicoding = unicoding;
        emit settingChanged("unicoding", unicoding);
    }
}

void
ProfileSettings::setKeymapName(const QString &keymapName)
{
    if (m_keymapName != keymapName || m_keymap == nullptr)
    {
        if (m_keymap) {
            disconnect(m_mocKeymap);
            m_keymap->putProfileReference();
        }

        m_keymap = g_settings->keymap(keymapName);
        m_keymap->takeProfileReference();
        m_keymap->activate();
        m_mocKeymap = connect(m_keymap, SIGNAL(destroyed()), SLOT(handleKeymapDestroyed()));

        m_keymapName = keymapName;
        emit keymapChanged(m_keymap);
        emit settingChanged("keymapName", keymapName);
        reportUpdate(m_row);
    }
}

void
ProfileSettings::setPalette(const QString &palette)
{
    if (m_palette != palette) {
        m_palette = palette;
        m_content.Termcolors::operator=(palette);
        emit paletteChanged(ChProf);
        emit settingChanged("palette", palette);
    }
}

void
ProfileSettings::setDircolors(const QString &dircolors)
{
    if (m_dircolors != dircolors) {
        m_dircolors = dircolors;
        m_content.Dircolors::operator=(dircolors);
        emit paletteChanged(ChProf);
        emit settingChanged("dircolors", dircolors);
    }
}

void
ProfileSettings::setIcon(const QString &icon)
{
    if (m_icon != icon) {
        m_icon = icon;

        m_nameIcon = !icon.isEmpty() ?
            ThumbIcon::getIcon(ThumbIcon::TerminalType, icon) :
            QIcon();

        emit iconChanged(ChProf);
        emit settingChanged("icon", icon);
        reportUpdate(m_row);
        g_settings->reportProfileIcon(this);
    }
}

void
ProfileSettings::setTermSize(QSize termSize)
{
    if (termSize.width() < TERM_MIN_COLS)
        termSize.setWidth(TERM_MIN_COLS);
    if (termSize.width() > TERM_MAX_COLS)
        termSize.setWidth(TERM_MAX_COLS);
    if (termSize.height() < TERM_MIN_ROWS)
        termSize.setHeight(TERM_MIN_ROWS);
    if (termSize.height() > TERM_MAX_ROWS)
        termSize.setHeight(TERM_MAX_ROWS);

    if (m_termSize != termSize) {
        m_termSize = termSize;
        emit settingChanged("termSize", termSize);
    }
}

void
ProfileSettings::setScrollbackSize(unsigned scrollbackSize)
{
    if (scrollbackSize > TERM_MAX_CAPORDER)
        scrollbackSize = TERM_MAX_CAPORDER;

    REG_SETTER_BODY(scrollbackSize)
}

void
ProfileSettings::setAutoClose(int autoClose)
{
    if (autoClose < Tsq::AutoCloseNever || autoClose > Tsq::AutoCloseAlways)
        autoClose = Tsq::AutoCloseExit;

    REG_SETTER_BODY(autoClose)
}

void
ProfileSettings::setPromptClose(int promptClose)
{
    if (promptClose < Tsqt::PromptCloseNever || promptClose > Tsqt::PromptCloseAlways)
        promptClose = Tsqt::PromptCloseSubproc;

    REG_SETTER_BODY(promptClose)
}

void
ProfileSettings::setRecentPrompts(int recentPrompts)
{
    if (recentPrompts < 0)
        recentPrompts = 0;
    if (recentPrompts > MAX_MINIMAP_PROMPTS)
        recentPrompts = MAX_MINIMAP_PROMPTS;

    SIG_SETTER_BODY(recentPrompts, miscSettingsChanged)
}

void
ProfileSettings::setFileStyle(int fileStyle)
{
    if (fileStyle < 1 || fileStyle > 2)
        fileStyle = 1;

    SIG_SETTER_BODY(fileStyle, miscSettingsChanged)
}

void
ProfileSettings::setFileEffect(int fileEffect)
{
    if (fileEffect < 0 || fileEffect > 2)
        fileEffect = 0;

    SIG_SETTER_BODY(fileEffect, fileSettingsChanged)
}

void
ProfileSettings::setExitEffect(int exitEffect)
{
    if (exitEffect < 0 || exitEffect > 2)
        exitEffect = 0;

    REG_SETTER_BODY(exitEffect)
}

REG_SETTER(ProfileSettings::setEnviron, environ, const QStringList &)
SIGARG_SETTER(ProfileSettings::setFont, font, const QString &, fontChanged, ChProf)
SIGARG_SETTER(ProfileSettings::setLayout, layout, const QString &, layoutChanged, ChProf)
SIGARG_SETTER(ProfileSettings::setFills, fills, const QString &, fillsChanged, ChProf)
SIGARG_SETTER(ProfileSettings::setBadge, badge, const QString &, badgeChanged, ChProf)
REG_SETTER(ProfileSettings::setDirectory, directory, const QString &)
REG_SETTER(ProfileSettings::setLang, lang, const QString &)
REG_SETTER(ProfileSettings::setMessage, message, const QString &)
REG_SETTER(ProfileSettings::setEmulatorFlags, emulatorFlags, qulonglong)
REG_SETTER(ProfileSettings::setFileLimit, fileLimit, unsigned)
REG_SETTER(ProfileSettings::setExitAction, exitAction, int)
REG_SETTER(ProfileSettings::setAutoCloseTime, autoCloseTime, int)
SIGARG_SETTER(ProfileSettings::setRemoteFont, remoteFont, int, remoteFontChanged, ChLevel)
SIGARG_SETTER(ProfileSettings::setRemoteColors, remoteColors, int, remoteColorsChanged, ChLevel)
SIGARG_SETTER(ProfileSettings::setRemoteLayout, remoteLayout, int, remoteLayoutChanged, ChLevel)
SIGARG_SETTER(ProfileSettings::setRemoteFills, remoteFills, int, remoteFillsChanged, ChLevel)
SIGARG_SETTER(ProfileSettings::setRemoteBadge, remoteBadge, int, remoteBadgeChanged, ChLevel)
REG_SETTER(ProfileSettings::setExitRuntime, exitRuntime, int)
SIG_SETTER(ProfileSettings::setMainIndicator, mainIndicator, bool, miscSettingsChanged)
SIG_SETTER(ProfileSettings::setThumbIndicator, thumbIndicator, bool, miscSettingsChanged)
SIG_SETTER(ProfileSettings::setShowIcon, showIcon, bool, miscSettingsChanged)
REG_SETTER(ProfileSettings::setSameDirectory, sameDirectory, bool)
REG_SETTER(ProfileSettings::setLangEnv, langEnv, bool)
REG_SETTER(ProfileSettings::setEncodingWarn, encodingWarn, bool)
REG_SETTER(ProfileSettings::setPromptNewline, promptNewline, bool)
REG_SETTER(ProfileSettings::setScrollClear, scrollClear, bool)
REG_SETTER(ProfileSettings::setFollowDefault, followDefault, bool)
NOTIFY_SETTER(ProfileSettings::setRemoteInput, remoteInput, bool)
REG_SETTER(ProfileSettings::setRemoteClipboard, remoteClipboard, bool)
REG_SETTER(ProfileSettings::setRemoteReset, remoteReset, bool)
SIG_SETTER(ProfileSettings::setShowDotfiles, showDotfiles, bool, fileSettingsChanged)
SIG_SETTER(ProfileSettings::setFileClassify, fileClassify, bool, fileSettingsChanged)
SIG_SETTER(ProfileSettings::setFileGittify, fileGittify, bool, fileSettingsChanged)
SIG_SETTER(ProfileSettings::setFileColorize, fileColorize, bool, fileSettingsChanged)
SIG_SETTER(ProfileSettings::setFileGitline, fileGitline, bool, miscSettingsChanged)
REG_SETTER(ProfileSettings::setDircolorsEnv, dircolorsEnv, bool)
SIG_SETTER(ProfileSettings::setFetchPos, fetchPos, bool, miscSettingsChanged)
SIG_SETTER(ProfileSettings::setMouseMode, mouseMode, bool, miscSettingsChanged)
SIG_SETTER(ProfileSettings::setFocusMode, focusMode, bool, miscSettingsChanged)
SIG_SETTER(ProfileSettings::setScrollMode, scrollMode, bool, miscSettingsChanged)
