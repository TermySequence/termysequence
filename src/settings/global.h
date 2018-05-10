// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base.h"
#include "app/color.h"

enum BellStyle {
    BellNone, BellFlash, BellBounce
};
enum FetchSpeed {
    SpeedNone = -1, SpeedMax = 0, SpeedNormal = 100, SpeedSlow = 1000
};
enum FocusEffect {
    FocusNever, FocusSplit, FocusAlways
};
enum SuggsAction {
    SuggActNothing, SuggActWrite, SuggActWriteNewline, SuggActCopy,
    SuggActRemove, SuggNAct
};
enum JobsAction {
    JobActNothing, JobActScrollStart, JobActScrollEnd, JobActCopyCommand,
    JobActWriteCommand, JobActWriteCommandNewline, JobActCopyOutput, JobActCopy,
    JobActSelectOutput, JobActSelectCommand, JobActSelect,
    JobNAct
};
enum NotesAction {
    NoteActNothing, NoteActScrollStart, NoteActScrollEnd, NoteActSelect,
    NoteActRemove, NoteNAct
};
enum FilesAction {
    FileActNothing, FileActOpen, FileActSmartOpen, FileActDownload,
    FileActWriteFile, FileActWriteDir, FileActCopyFile, FileActCopyDir,
    FileActUploadFile, FileActUploadDir, FileActRename, FileActDelete, FileNAct
};
enum TasksAction {
    TaskActNothing,
    TaskActFileDesk, TaskActFile, TaskActDirDesk, TaskActDir, TaskActTerm,
    TaskNLaunchAct, TaskActInspect, TaskActCancel, TaskActRestart, TaskNAct
};
enum DropAction {
    DropAsk, DropNothing, DropUploadHome, DropUploadTmp, DropUploadCwd,
    DropPasteName, DropPasteContent, DropNAct
};
enum InlineAction {
    InlineActNothing, InlineActOpen, InlineActSelect, InlineActCopy,
    InlineActWrite, InlineNAct
};

class GlobalSettings final: public SettingsBase
{
    Q_OBJECT
    // All Visible
    Q_PROPERTY(QString termCaption READ termCaption WRITE setTermCaption NOTIFY termCaptionChanged)
    Q_PROPERTY(QString termTooltip READ termTooltip WRITE setTermTooltip NOTIFY termTooltipChanged)
    Q_PROPERTY(QString serverCaption READ serverCaption WRITE setServerCaption NOTIFY serverCaptionChanged)
    Q_PROPERTY(QString serverTooltip READ serverTooltip WRITE setServerTooltip NOTIFY serverTooltipChanged)
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle NOTIFY windowTitleChanged)
    Q_PROPERTY(QString jobFont READ jobFont WRITE setJobFont NOTIFY jobFontChanged)
    Q_PROPERTY(QString downloadLocation READ downloadLocation WRITE setDownloadLocation)
    Q_PROPERTY(QString taskFile READ taskFile WRITE setTaskFile)
    Q_PROPERTY(QString taskDir READ taskDir WRITE setTaskDir)
    Q_PROPERTY(QString taskProfile READ taskProfile WRITE setTaskProfile)
    Q_PROPERTY(QString termAction0 READ termAction0  WRITE setTermAction0)
    Q_PROPERTY(QString termAction1 READ termAction1  WRITE setTermAction1)
    Q_PROPERTY(QString termAction2 READ termAction2  WRITE setTermAction2)
    Q_PROPERTY(QString termAction3 READ termAction3  WRITE setTermAction3)
    Q_PROPERTY(QString serverAction0 READ serverAction0  WRITE setServerAction0)
    Q_PROPERTY(QString serverAction1 READ serverAction1  WRITE setServerAction1)
    Q_PROPERTY(QString serverAction2 READ serverAction2  WRITE setServerAction2)
    Q_PROPERTY(QString serverAction3 READ serverAction3  WRITE setServerAction3)
    Q_PROPERTY(QString docRoot READ docRoot WRITE setDocRoot)
    Q_PROPERTY(QString iconTheme READ iconTheme WRITE setIconTheme)
    Q_PROPERTY(int logThreshold READ logThreshold WRITE setLogThreshold)
    Q_PROPERTY(int menuSize READ menuSize WRITE setMenuSize NOTIFY menuSizeChanged)
    Q_PROPERTY(unsigned cursorBlinks READ cursorBlinks WRITE setCursorBlinks NOTIFY cursorBlinksChanged)
    Q_PROPERTY(unsigned textBlinks READ textBlinks WRITE setTextBlinks NOTIFY textBlinksChanged)
    Q_PROPERTY(unsigned skipBlinks READ skipBlinks WRITE setSkipBlinks NOTIFY skipBlinksChanged)
    Q_PROPERTY(unsigned blinkTime READ blinkTime WRITE setBlinkTime NOTIFY blinkTimeChanged)
    Q_PROPERTY(unsigned populateTime READ populateTime WRITE setPopulateTime)
    Q_PROPERTY(unsigned taskTime READ taskTime WRITE setTaskTime)
    Q_PROPERTY(unsigned suggTime READ suggTime WRITE setSuggTime)
    Q_PROPERTY(unsigned peekTime READ peekTime WRITE setPeekTime)
    Q_PROPERTY(unsigned badgeRate READ badgeRate WRITE setBadgeRate NOTIFY mainSettingsChanged)
    Q_PROPERTY(int mainBell READ mainBell WRITE setMainBell)
    Q_PROPERTY(int thumbBell READ thumbBell WRITE setThumbBell)
    Q_PROPERTY(int jobLimit READ jobLimit WRITE setJobLimit)
    Q_PROPERTY(int taskLimit READ taskLimit WRITE setTaskLimit)
    Q_PROPERTY(int fetchSpeed READ fetchSpeed WRITE setFetchSpeed NOTIFY fetchSpeedChanged)
    Q_PROPERTY(int focusEffect READ focusEffect WRITE setFocusEffect NOTIFY focusEffectChanged)
    Q_PROPERTY(int suggAction0 READ suggAction0 WRITE setSuggAction0)
    Q_PROPERTY(int suggAction1 READ suggAction1 WRITE setSuggAction1)
    Q_PROPERTY(int suggAction2 READ suggAction2 WRITE setSuggAction2)
    Q_PROPERTY(int suggAction3 READ suggAction3 WRITE setSuggAction3)
    Q_PROPERTY(int jobAction0 READ jobAction0 WRITE setJobAction0)
    Q_PROPERTY(int jobAction1 READ jobAction1 WRITE setJobAction1)
    Q_PROPERTY(int jobAction2 READ jobAction2 WRITE setJobAction2)
    Q_PROPERTY(int jobAction3 READ jobAction3 WRITE setJobAction3)
    Q_PROPERTY(int noteAction0 READ noteAction0 WRITE setNoteAction0)
    Q_PROPERTY(int noteAction1 READ noteAction1 WRITE setNoteAction1)
    Q_PROPERTY(int noteAction2 READ noteAction2 WRITE setNoteAction2)
    Q_PROPERTY(int noteAction3 READ noteAction3 WRITE setNoteAction3)
    Q_PROPERTY(int fileAction0 READ fileAction0 WRITE setFileAction0)
    Q_PROPERTY(int fileAction1 READ fileAction1 WRITE setFileAction1)
    Q_PROPERTY(int fileAction2 READ fileAction2 WRITE setFileAction2)
    Q_PROPERTY(int fileAction3 READ fileAction3 WRITE setFileAction3)
    Q_PROPERTY(int taskAction0 READ taskAction0 WRITE setTaskAction0)
    Q_PROPERTY(int taskAction1 READ taskAction1 WRITE setTaskAction1)
    Q_PROPERTY(int taskAction2 READ taskAction2 WRITE setTaskAction2)
    Q_PROPERTY(int taskAction3 READ taskAction3 WRITE setTaskAction3)
    Q_PROPERTY(int inlineAction0 READ inlineAction0 WRITE setInlineAction0)
    Q_PROPERTY(int inlineAction1 READ inlineAction1 WRITE setInlineAction1)
    Q_PROPERTY(int inlineAction2 READ inlineAction2 WRITE setInlineAction2)
    Q_PROPERTY(int inlineDrag READ inlineDrag WRITE setInlineDrag)
    Q_PROPERTY(int uploadConfig READ uploadConfig WRITE setUploadConfig)
    Q_PROPERTY(int downloadConfig READ downloadConfig WRITE setDownloadConfig)
    Q_PROPERTY(int deleteConfig READ deleteConfig WRITE setDeleteConfig)
    Q_PROPERTY(int renameConfig READ renameConfig WRITE setRenameConfig)
    Q_PROPERTY(int downloadAction READ downloadAction WRITE setDownloadAction)
    Q_PROPERTY(int mountAction READ mountAction WRITE setMountAction)
    Q_PROPERTY(int termLocalFile READ termLocalFile WRITE setTermLocalFile)
    Q_PROPERTY(int termRemoteFile READ termRemoteFile WRITE setTermRemoteFile)
    Q_PROPERTY(int thumbLocalFile READ thumbLocalFile WRITE setThumbLocalFile)
    Q_PROPERTY(int thumbRemoteFile READ thumbRemoteFile WRITE setThumbRemoteFile)
    Q_PROPERTY(int serverFile READ serverFile WRITE setServerFile)
    Q_PROPERTY(int presFontSize READ presFontSize WRITE setPresFontSize)
    Q_PROPERTY(bool logToSystem READ logToSystem WRITE setLogToSystem)
    Q_PROPERTY(bool commandMode READ commandMode WRITE setCommandMode)
    Q_PROPERTY(bool menuBar READ menuBar WRITE setMenuBar)
    Q_PROPERTY(bool statusBar READ statusBar WRITE setStatusBar)
    Q_PROPERTY(bool saveGeometry READ saveGeometry WRITE setSaveGeometry)
    Q_PROPERTY(bool restoreGeometry READ restoreGeometry WRITE setRestoreGeometry)
    Q_PROPERTY(bool saveOrder READ saveOrder WRITE setSaveOrder)
    Q_PROPERTY(bool restoreOrder READ restoreOrder WRITE setRestoreOrder)
    Q_PROPERTY(bool cursorBlink READ cursorBlink WRITE setCursorBlink NOTIFY cursorBlinkChanged)
    Q_PROPERTY(bool textBlink READ textBlink WRITE setTextBlink NOTIFY textBlinkChanged)
    Q_PROPERTY(bool resizeEffect READ resizeEffect WRITE setResizeEffect NOTIFY resizeEffectChanged)
    Q_PROPERTY(bool badgeEffect READ badgeEffect WRITE setBadgeEffect NOTIFY mainSettingsChanged)
    Q_PROPERTY(bool mainIndex READ mainIndex WRITE setMainIndex NOTIFY mainSettingsChanged)
    Q_PROPERTY(bool thumbIndex READ thumbIndex WRITE setThumbIndex NOTIFY thumbIndexChanged)
    Q_PROPERTY(bool denseThumb READ denseThumb WRITE setDenseThumb NOTIFY denseThumbChanged)
    Q_PROPERTY(bool launchTransient READ launchTransient WRITE setLaunchTransient)
    Q_PROPERTY(bool launchPersistent READ launchPersistent WRITE setLaunchPersistent)
    Q_PROPERTY(bool preferTransient READ preferTransient WRITE setPreferTransient NOTIFY preferTransientChanged)
    Q_PROPERTY(bool closeOnLaunch READ closeOnLaunch WRITE setCloseOnLaunch)
    Q_PROPERTY(bool showPeek READ showPeek WRITE setShowPeek)
    Q_PROPERTY(bool raiseTerminals READ raiseTerminals WRITE setRaiseTerminals)
    Q_PROPERTY(bool raiseSuggestions READ raiseSuggestions WRITE setRaiseSuggestions)
    Q_PROPERTY(bool raiseFiles READ raiseFiles WRITE setRaiseFiles)
    Q_PROPERTY(bool raiseTasks READ raiseTasks WRITE setRaiseTasks)
    Q_PROPERTY(bool autoShowTask READ autoShowTask WRITE setAutoShowTask)
    Q_PROPERTY(bool autoHideTask READ autoHideTask WRITE setAutoHideTask)
    Q_PROPERTY(bool autoQuit READ autoQuit WRITE setAutoQuit)
    Q_PROPERTY(bool presFullScreen READ presFullScreen WRITE setPresFullScreen)
    Q_PROPERTY(bool presMenuBar READ presMenuBar WRITE setPresMenuBar)
    Q_PROPERTY(bool presStatusBar READ presStatusBar WRITE setPresStatusBar)
    Q_PROPERTY(bool presTools READ presTools WRITE setPresTools)
    Q_PROPERTY(bool presIndex READ presIndex WRITE setPresIndex NOTIFY mainSettingsChanged)
    Q_PROPERTY(bool presIndicator READ presIndicator WRITE setPresIndicator NOTIFY mainSettingsChanged)
    Q_PROPERTY(bool presBadge READ presBadge WRITE setPresBadge NOTIFY mainSettingsChanged)
    Q_PROPERTY(bool raiseSelect READ raiseSelect WRITE setRaiseSelect)
    Q_PROPERTY(bool raiseCommand READ raiseCommand WRITE setRaiseCommand)
    Q_PROPERTY(bool autoSelect READ autoSelect WRITE setAutoSelect)
    Q_PROPERTY(bool copySelect READ copySelect WRITE setCopySelect)
    Q_PROPERTY(bool writeSelect READ writeSelect WRITE setWriteSelect)
    Q_PROPERTY(bool inputSelect READ inputSelect WRITE setInputSelect)
    Q_PROPERTY(bool renderImages READ renderImages WRITE setRenderImages)
    Q_PROPERTY(bool allowLinks READ allowLinks WRITE setAllowLinks)
    Q_PROPERTY(bool renderAvatars READ renderAvatars WRITE setRenderAvatars)
    Q_PROPERTY(bool localDownload READ localDownload WRITE setLocalDownload)

    REFPROP(QString, termCaption, setTermCaption)
    REFPROP(QString, termTooltip, setTermTooltip)
    REFPROP(QString, serverCaption, setServerCaption)
    REFPROP(QString, serverTooltip, setServerTooltip)
    REFPROP(QString, windowTitle, setWindowTitle)
    REFPROP(QString, jobFont, setJobFont)
    REFPROP(QString, downloadLocation, setDownloadLocation)
    REFPROP(QString, taskFile, setTaskFile)
    REFPROP(QString, taskDir, setTaskDir)
    REFPROP(QString, taskProfile, setTaskProfile)
    REFPROP(QString, termAction0, setTermAction0)
    REFPROP(QString, termAction1, setTermAction1)
    REFPROP(QString, termAction2, setTermAction2)
    REFPROP(QString, termAction3, setTermAction3)
    REFPROP(QString, serverAction0, setServerAction0)
    REFPROP(QString, serverAction1, setServerAction1)
    REFPROP(QString, serverAction2, setServerAction2)
    REFPROP(QString, serverAction3, setServerAction3)
    VALPROP(QString, docRoot, setDocRoot)
    REFPROP(QString, iconTheme, setIconTheme)
    VALPROP(int, logThreshold, setLogThreshold)
    VALPROP(int, menuSize, setMenuSize)
    VALPROP(unsigned, cursorBlinks, setCursorBlinks)
    VALPROP(unsigned, textBlinks, setTextBlinks)
    VALPROP(unsigned, skipBlinks, setSkipBlinks)
    VALPROP(unsigned, blinkTime, setBlinkTime)
    VALPROP(unsigned, populateTime, setPopulateTime)
    VALPROP(unsigned, taskTime, setTaskTime)
    VALPROP(unsigned, suggTime, setSuggTime)
    VALPROP(unsigned, peekTime, setPeekTime)
    VALPROP(unsigned, badgeRate, setBadgeRate)
    VALPROP(int, mainBell, setMainBell)
    VALPROP(int, thumbBell, setThumbBell)
    VALPROP(int, jobLimit, setJobLimit)
    VALPROP(int, taskLimit, setTaskLimit)
    VALPROP(int, fetchSpeed, setFetchSpeed)
    VALPROP(int, focusEffect, setFocusEffect)
    VALPROP(int, suggAction0, setSuggAction0)
    VALPROP(int, suggAction1, setSuggAction1)
    VALPROP(int, suggAction2, setSuggAction2)
    VALPROP(int, suggAction3, setSuggAction3)
    VALPROP(int, jobAction0, setJobAction0)
    VALPROP(int, jobAction1, setJobAction1)
    VALPROP(int, jobAction2, setJobAction2)
    VALPROP(int, jobAction3, setJobAction3)
    VALPROP(int, noteAction0, setNoteAction0)
    VALPROP(int, noteAction1, setNoteAction1)
    VALPROP(int, noteAction2, setNoteAction2)
    VALPROP(int, noteAction3, setNoteAction3)
    VALPROP(int, fileAction0, setFileAction0)
    VALPROP(int, fileAction1, setFileAction1)
    VALPROP(int, fileAction2, setFileAction2)
    VALPROP(int, fileAction3, setFileAction3)
    VALPROP(int, taskAction0, setTaskAction0)
    VALPROP(int, taskAction1, setTaskAction1)
    VALPROP(int, taskAction2, setTaskAction2)
    VALPROP(int, taskAction3, setTaskAction3)
    VALPROP(int, inlineAction0, setInlineAction0)
    VALPROP(int, inlineAction1, setInlineAction1)
    VALPROP(int, inlineAction2, setInlineAction2)
    VALPROP(int, inlineDrag, setInlineDrag)
    VALPROP(int, uploadConfig, setUploadConfig)
    VALPROP(int, downloadConfig, setDownloadConfig)
    VALPROP(int, deleteConfig, setDeleteConfig)
    VALPROP(int, renameConfig, setRenameConfig)
    VALPROP(int, downloadAction, setDownloadAction)
    VALPROP(int, mountAction, setMountAction)
    VALPROP(int, termLocalFile, setTermLocalFile)
    VALPROP(int, termRemoteFile, setTermRemoteFile)
    VALPROP(int, thumbLocalFile, setThumbLocalFile)
    VALPROP(int, thumbRemoteFile, setThumbRemoteFile)
    VALPROP(int, serverFile, setServerFile)
    VALPROP(int, presFontSize, setPresFontSize)
    VALPROP(bool, logToSystem, setLogToSystem)
    VALPROP(bool, commandMode, setCommandMode)
    VALPROP(bool, menuBar, setMenuBar)
    VALPROP(bool, statusBar, setStatusBar)
    VALPROP(bool, saveGeometry, setSaveGeometry)
    VALPROP(bool, restoreGeometry, setRestoreGeometry)
    VALPROP(bool, saveOrder, setSaveOrder)
    VALPROP(bool, restoreOrder, setRestoreOrder)
    VALPROP(bool, cursorBlink, setCursorBlink)
    VALPROP(bool, textBlink, setTextBlink)
    VALPROP(bool, resizeEffect, setResizeEffect)
    VALPROP(bool, badgeEffect, setBadgeEffect)
    VALPROP(bool, mainIndex, setMainIndex)
    VALPROP(bool, thumbIndex, setThumbIndex)
    VALPROP(bool, denseThumb, setDenseThumb)
    VALPROP(bool, launchTransient, setLaunchTransient)
    VALPROP(bool, launchPersistent, setLaunchPersistent)
    VALPROP(bool, preferTransient, setPreferTransient)
    VALPROP(bool, closeOnLaunch, setCloseOnLaunch)
    VALPROP(bool, showPeek, setShowPeek)
    VALPROP(bool, raiseTerminals, setRaiseTerminals)
    VALPROP(bool, raiseSuggestions, setRaiseSuggestions)
    VALPROP(bool, raiseFiles, setRaiseFiles)
    VALPROP(bool, raiseTasks, setRaiseTasks)
    VALPROP(bool, autoShowTask, setAutoShowTask)
    VALPROP(bool, autoHideTask, setAutoHideTask)
    VALPROP(bool, autoQuit, setAutoQuit)
    VALPROP(bool, presFullScreen, setPresFullScreen)
    VALPROP(bool, presMenuBar, setPresMenuBar)
    VALPROP(bool, presStatusBar, setPresStatusBar)
    VALPROP(bool, presTools, setPresTools)
    VALPROP(bool, presIndex, setPresIndex)
    VALPROP(bool, presIndicator, setPresIndicator)
    VALPROP(bool, presBadge, setPresBadge)
    VALPROP(bool, raiseSelect, setRaiseSelect)
    VALPROP(bool, raiseCommand, setRaiseCommand)
    VALPROP(bool, autoSelect, setAutoSelect)
    VALPROP(bool, copySelect, setCopySelect)
    VALPROP(bool, writeSelect, setWriteSelect)
    VALPROP(bool, inputSelect, setInputSelect)
    VALPROP(bool, renderImages, setRenderImages)
    VALPROP(bool, allowLinks, setAllowLinks)
    VALPROP(bool, renderAvatars, setRenderAvatars)
    VALPROP(bool, localDownload, setLocalDownload)

private:
    // Colors
    QColor m_colors[NColors];
    void initColors();

signals:
    void termCaptionChanged(QString termCaption);
    void termTooltipChanged(QString termTooltip);
    void serverCaptionChanged(QString serverCaption);
    void serverTooltipChanged(QString serverTooltip);
    void windowTitleChanged(QString windowTitle);
    void jobFontChanged(QString jobFont);
    void menuSizeChanged();
    void cursorBlinksChanged(unsigned cursorBlinks);
    void textBlinksChanged(unsigned textBlinks);
    void skipBlinksChanged(unsigned skipBlinks);
    void blinkTimeChanged(unsigned blinkTime);
    void fetchSpeedChanged(int fetchSpeed);
    void focusEffectChanged(int focusEffect);
    void cursorBlinkChanged(bool cursorBlink);
    void textBlinkChanged(bool textBlink);
    void resizeEffectChanged(bool resizeEffect);
    void thumbIndexChanged(bool thumbIndex);
    void denseThumbChanged();
    void preferTransientChanged();
    void mainSettingsChanged();

    void actionSaveGeometry();
    void actionSaveOrder();

    // Emitted from settings
    void iconRulesChanged();
    void avatarsChanged();

public:
    GlobalSettings(const QString &path = QString());
    static void populateDefaults();

    // Colors
    inline const QColor& color(ColorName name) const { return m_colors[name]; }
    inline void setColor(ColorName name, const QColor &val) { m_colors[name] = val; }

    // Documentation links
    QString docUrl(const QString &page) const;
    QString docLink(const char *page, const char *text) const;

public slots:
    void requestSaveGeometry();
    void requestSaveOrder();
};

extern GlobalSettings *g_global;
