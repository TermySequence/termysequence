// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"
#include "base.h"
#include "palette.h"

#include <QSize>
#include <QIcon>

class TermKeymap;

enum FileStyle { FilesLong = 1, FilesShort = 2 };
enum WindowType { WindowNever, WindowFocus, WindowAlways };
enum ChangeType { ChProf, ChPref, ChSess, ChLevel };

class ProfileSettings final: public SettingsBase
{
    Q_OBJECT
    // All Visible
    Q_PROPERTY(QStringList command READ command WRITE setCommand)
    Q_PROPERTY(QStringList environ READ environ WRITE setEnviron)
    Q_PROPERTY(QStringList unicoding READ unicoding WRITE setUnicoding)
    Q_PROPERTY(QString keymapName READ keymapName WRITE setKeymapName)
    Q_PROPERTY(QString palette READ palette WRITE setPalette NOTIFY paletteChanged)
    Q_PROPERTY(QString dircolors READ dircolors WRITE setDircolors NOTIFY paletteChanged)
    Q_PROPERTY(QString font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QString layout READ layout WRITE setLayout NOTIFY layoutChanged)
    Q_PROPERTY(QString fills READ fills WRITE setFills NOTIFY fillsChanged)
    Q_PROPERTY(QString badge READ badge WRITE setBadge NOTIFY badgeChanged)
    Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)
    Q_PROPERTY(QString directory READ directory WRITE setDirectory)
    Q_PROPERTY(QString lang READ lang WRITE setLang)
    Q_PROPERTY(QString message READ message WRITE setMessage)
    Q_PROPERTY(QSize termSize READ termSize WRITE setTermSize)
    Q_PROPERTY(qulonglong emulatorFlags READ emulatorFlags WRITE setEmulatorFlags)
    Q_PROPERTY(unsigned scrollbackSize READ scrollbackSize WRITE setScrollbackSize)
    Q_PROPERTY(unsigned fileLimit READ fileLimit WRITE setFileLimit)
    Q_PROPERTY(int exitAction READ exitAction WRITE setExitAction)
    Q_PROPERTY(int autoClose READ autoClose WRITE setAutoClose)
    Q_PROPERTY(int autoCloseTime READ autoCloseTime WRITE setAutoCloseTime)
    Q_PROPERTY(int promptClose READ promptClose WRITE setPromptClose)
    Q_PROPERTY(int remoteFont READ remoteFont WRITE setRemoteFont NOTIFY remoteFontChanged)
    Q_PROPERTY(int remoteColors READ remoteColors WRITE setRemoteColors NOTIFY remoteColorsChanged)
    Q_PROPERTY(int remoteLayout READ remoteLayout WRITE setRemoteLayout NOTIFY remoteLayoutChanged)
    Q_PROPERTY(int remoteFills READ remoteFills WRITE setRemoteFills NOTIFY remoteFillsChanged)
    Q_PROPERTY(int remoteBadge READ remoteBadge WRITE setRemoteBadge NOTIFY remoteBadgeChanged)
    Q_PROPERTY(int recentPrompts READ recentPrompts WRITE setRecentPrompts NOTIFY miscSettingsChanged)
    Q_PROPERTY(int fileStyle READ fileStyle WRITE setFileStyle NOTIFY miscSettingsChanged)
    Q_PROPERTY(int fileEffect READ fileEffect WRITE setFileEffect NOTIFY fileSettingsChanged)
    Q_PROPERTY(int exitEffect READ exitEffect WRITE setExitEffect)
    Q_PROPERTY(int exitRuntime READ exitRuntime WRITE setExitRuntime)
    Q_PROPERTY(bool mainIndicator READ mainIndicator WRITE setMainIndicator NOTIFY miscSettingsChanged)
    Q_PROPERTY(bool thumbIndicator READ thumbIndicator WRITE setThumbIndicator NOTIFY miscSettingsChanged)
    Q_PROPERTY(bool showIcon READ showIcon WRITE setShowIcon NOTIFY miscSettingsChanged)
    Q_PROPERTY(bool sameDirectory READ sameDirectory WRITE setSameDirectory)
    Q_PROPERTY(bool langEnv READ langEnv WRITE setLangEnv)
    Q_PROPERTY(bool encodingWarn READ encodingWarn WRITE setEncodingWarn)
    Q_PROPERTY(bool promptNewline READ promptNewline WRITE setPromptNewline)
    Q_PROPERTY(bool scrollClear READ scrollClear WRITE setScrollClear)
    Q_PROPERTY(bool followDefault READ followDefault WRITE setFollowDefault)
    Q_PROPERTY(bool remoteInput READ remoteInput WRITE setRemoteInput NOTIFY remoteInputChanged)
    Q_PROPERTY(bool remoteClipboard READ remoteClipboard WRITE setRemoteClipboard)
    Q_PROPERTY(bool remoteReset READ remoteReset WRITE setRemoteReset)
    Q_PROPERTY(bool showDotfiles READ showDotfiles WRITE setShowDotfiles NOTIFY fileSettingsChanged)
    Q_PROPERTY(bool fileClassify READ fileClassify WRITE setFileClassify NOTIFY fileSettingsChanged)
    Q_PROPERTY(bool fileGittify READ fileGittify WRITE setFileGittify NOTIFY fileSettingsChanged)
    Q_PROPERTY(bool fileColorize READ fileColorize WRITE setFileColorize NOTIFY fileSettingsChanged)
    Q_PROPERTY(bool fileGitline READ fileGitline WRITE setFileGitline NOTIFY miscSettingsChanged)
    Q_PROPERTY(bool dircolorsEnv READ dircolorsEnv WRITE setDircolorsEnv)
    Q_PROPERTY(bool fetchPos READ fetchPos WRITE setFetchPos NOTIFY miscSettingsChanged)
    Q_PROPERTY(bool mouseMode READ mouseMode WRITE setMouseMode NOTIFY miscSettingsChanged)
    Q_PROPERTY(bool focusMode READ focusMode WRITE setFocusMode NOTIFY miscSettingsChanged)
    Q_PROPERTY(bool scrollMode READ scrollMode WRITE setScrollMode NOTIFY miscSettingsChanged)

private:
    // Non-properties
    TermKeymap *m_keymap = nullptr;
    QMetaObject::Connection m_mocKeymap;

    TermPalette m_content;
    QIcon m_nameIcon;

    // Properties
    REFPROP(QStringList, command, setCommand)
    REFPROP(QStringList, environ, setEnviron)
    REFPROP(QStringList, unicoding, setUnicoding)
    REFPROP(QString, keymapName, setKeymapName)
    REFPROP(QString, palette, setPalette)
    REFPROP(QString, dircolors, setDircolors)
    REFPROP(QString, font, setFont)
    REFPROP(QString, layout, setLayout)
    REFPROP(QString, fills, setFills)
    REFPROP(QString, badge, setBadge)
    REFPROP(QString, icon, setIcon)
    REFPROP(QString, directory, setDirectory)
    REFPROP(QString, lang, setLang)
    REFPROP(QString, message, setMessage)
    VALPROP(QSize, termSize, setTermSize)
    VALPROP(qulonglong, emulatorFlags, setEmulatorFlags)
    VALPROP(unsigned, scrollbackSize, setScrollbackSize)
    VALPROP(unsigned, fileLimit, setFileLimit)
    VALPROP(int, exitAction, setExitAction)
    VALPROP(int, autoClose, setAutoClose)
    VALPROP(int, autoCloseTime, setAutoCloseTime)
    VALPROP(int, promptClose, setPromptClose)
    VALPROP(int, remoteFont, setRemoteFont)
    VALPROP(int, remoteColors, setRemoteColors)
    VALPROP(int, remoteLayout, setRemoteLayout)
    VALPROP(int, remoteFills, setRemoteFills)
    VALPROP(int, remoteBadge, setRemoteBadge)
    VALPROP(int, recentPrompts, setRecentPrompts)
    VALPROP(int, fileStyle, setFileStyle)
    VALPROP(int, fileEffect, setFileEffect)
    VALPROP(int, exitEffect, setExitEffect)
    VALPROP(int, exitRuntime, setExitRuntime)
    VALPROP(bool, mainIndicator, setMainIndicator)
    VALPROP(bool, thumbIndicator, setThumbIndicator)
    VALPROP(bool, showIcon, setShowIcon)
    VALPROP(bool, sameDirectory, setSameDirectory)
    VALPROP(bool, langEnv, setLangEnv)
    VALPROP(bool, encodingWarn, setEncodingWarn)
    VALPROP(bool, promptNewline, setPromptNewline)
    VALPROP(bool, scrollClear, setScrollClear)
    VALPROP(bool, followDefault, setFollowDefault)
    VALPROP(bool, remoteInput, setRemoteInput)
    VALPROP(bool, remoteClipboard, setRemoteClipboard)
    VALPROP(bool, remoteReset, setRemoteReset)
    VALPROP(bool, showDotfiles, setShowDotfiles)
    VALPROP(bool, fileClassify, setFileClassify)
    VALPROP(bool, fileGittify, setFileGittify)
    VALPROP(bool, fileColorize, setFileColorize)
    VALPROP(bool, fileGitline, setFileGitline)
    VALPROP(bool, dircolorsEnv, setDircolorsEnv)
    VALPROP(bool, fetchPos, setFetchPos)
    VALPROP(bool, mouseMode, setMouseMode)
    VALPROP(bool, focusMode, setFocusMode)
    VALPROP(bool, scrollMode, setScrollMode)

signals:
    // Non-property signals
    void keymapChanged(TermKeymap *keymap);

    // Property signals
    void fontChanged(int chtype);
    void paletteChanged(int chtype);
    void layoutChanged(int chtype);
    void fillsChanged(int chtype);
    void badgeChanged(int chtype);
    void iconChanged(int chtype);
    void remoteFontChanged(int chtype);
    void remoteColorsChanged(int chtype);
    void remoteLayoutChanged(int chtype);
    void remoteFillsChanged(int chtype);
    void remoteBadgeChanged(int chtype);
    void remoteInputChanged(bool remoteInput);
    void miscSettingsChanged();
    void fileSettingsChanged();

private slots:
    void handleKeymapDestroyed();

public:
    ProfileSettings(const QString &name, const QString &path = QString());
    ~ProfileSettings();
    static void populateDefaults(const QString &defaultFont);

    // Non-properties
    inline const TermPalette& content() const { return m_content; }
    inline const QIcon& nameIcon() const { return m_nameIcon; }
    inline TermKeymap* keymap() { return m_keymap; }

    void setFavorite(unsigned favorite);
    void setDefault(bool isdefault);

    inline int refcount() const { return m_refcount; }
    void takeReference();
    void putReference();

    // Utilities
    void toAttributes(AttributeMap &map) const;
    std::pair<QString,QString> toAttribute(const char *property) const;
    void fromAttributes(const AttributeMap &map);
};
