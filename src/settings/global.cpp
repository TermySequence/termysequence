// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "app/format.h"
#include "global.h"
#include "basemacros.h"
#include "choicewidget.h"
#include "checkwidget.h"
#include "actionwidget.h"
#include "intwidget.h"
#include "formatwidget.h"
#include "fontselect.h"
#include "launchselect.h"
#include "profileselect.h"
#include "downloadwidget.h"
#include "imagewidget.h"
#include "slotwidget.h"
#include "inputwidget.h"
#include "iconselect.h"
#include "lib/enums.h"

#include <QLoggingCategory>

GlobalSettings *g_global;

static const ChoiceDef s_logThresholdArg[] = {
    { TN("settings-enum", "On errors and warnings"), QtWarningMsg },
    { TN("settings-enum", "On errors only"), QtCriticalMsg },
    { TN("settings-enum", "Never"), QtFatalMsg },
    { NULL }
};

static const ChoiceDef s_mainBellStyle[] = {
    { TN("settings-enum", "None"), BellNone },
    { TN("settings-enum", "Flash"), BellFlash },
    { TN("settings-enum", "Cursor Bounce"), BellBounce },
    { NULL }
};

static const ChoiceDef s_thumbBellStyle[] = {
    { TN("settings-enum", "None"), BellNone },
    { TN("settings-enum", "Flash"), BellFlash },
    { NULL }
};

static const ChoiceDef s_fetchSpeed[] = {
    { TN("settings-enum", "Aggressively"), SpeedMax },
    { TN("settings-enum", "Normal Speed"), SpeedNormal },
    { TN("settings-enum", "Slowly"), SpeedSlow },
    { TN("settings-enum", "Never"), SpeedNone },
    { NULL }
};

static const ChoiceDef s_focusEffect[] = {
    { TN("settings-enum", "Always"), FocusAlways },
    { TN("settings-enum", "Split Windows Only"), FocusSplit },
    { TN("settings-enum", "Never"), FocusNever },
    { NULL }
};

static const ChoiceDef s_suggAction[] = {
    { TN("settings-enum", "Do nothing"), SuggActNothing },
    { TN("settings-enum", "Write command to terminal"), SuggActWrite },
    { TN("settings-enum", "Write command to terminal with newline"), SuggActWriteNewline },
    { TN("settings-enum", "Copy command to clipboard"), SuggActCopy },
    { TN("settings-enum", "Remove command from database"), SuggActRemove },
    { NULL }
};

static const ChoiceDef s_jobAction[] = {
    { TN("settings-enum", "Do nothing"), JobActNothing },
    { TN("settings-enum", "Scroll to beginning of job"), JobActScrollStart },
    { TN("settings-enum", "Scroll to end of job"), JobActScrollEnd },
    { TN("settings-enum", "Copy command to clipboard"), JobActCopyCommand },
    { TN("settings-enum", "Write command to terminal"), JobActWriteCommand },
    { TN("settings-enum", "Write command to terminal with newline"), JobActWriteCommandNewline },
    { TN("settings-enum", "Copy output to clipboard"), JobActCopyOutput },
    { TN("settings-enum", "Copy entire job to clipboard"), JobActCopy },
    { TN("settings-enum", "Select output"), JobActSelectOutput },
    { TN("settings-enum", "Select command"), JobActSelectCommand },
    { TN("settings-enum", "Select entire job"), JobActSelect },
    { NULL }
};

static const ChoiceDef s_noteAction[] = {
    { TN("settings-enum", "Do nothing"), NoteActNothing },
    { TN("settings-enum", "Scroll to beginning of note"), NoteActScrollStart },
    { TN("settings-enum", "Scroll to end of note"), NoteActScrollEnd },
    { TN("settings-enum", "Select annotation"), NoteActSelect },
    { TN("settings-enum", "Remove annotation"), NoteActRemove },
    { NULL }
};

static const ChoiceDef s_fileAction[] = {
    { TN("settings-enum", "Do nothing"), FileActNothing },
    { TN("settings-enum", "Open file using default launcher"), FileActOpen },
    { TN("settings-enum", "Open file or write cd command to terminal"), FileActSmartOpen },
    { TN("settings-enum", "Download file"), FileActDownload },
    { TN("settings-enum", "Write path to terminal"), FileActWriteFile },
    { TN("settings-enum", "Write parent path to terminal"), FileActWriteDir },
    { TN("settings-enum", "Copy path to clipboard"), FileActCopyFile },
    { TN("settings-enum", "Copy parent path to clipboard"), FileActCopyDir },
    { TN("settings-enum", "Upload to file"), FileActUploadFile },
    { TN("settings-enum", "Upload to parent folder"), FileActUploadDir },
    { TN("settings-enum", "Rename file"), FileActRename },
    { TN("settings-enum", "Delete file"), FileActDelete },
    { NULL }
};

static const ChoiceDef s_taskAction[] = {
    { TN("settings-enum", "Do nothing"), TaskActNothing },
    { TN("settings-enum", "Show task status information"), TaskActInspect },
    { TN("settings-enum", "Cancel task"), TaskActCancel },
    { TN("settings-enum", "Start another copy of task"), TaskActRestart },
    { TN("settings-enum", "Open file via local desktop environment"), TaskActFileDesk },
    { TN("settings-enum", "Open file using preferred launcher"), TaskActFile },
    { TN("settings-enum", "Open folder via local desktop environment"), TaskActDirDesk },
    { TN("settings-enum", "Open folder using preferred launcher"), TaskActDir },
    { TN("settings-enum", "Open folder in a new terminal"), TaskActTerm },
    { NULL }
};

static const ChoiceDef s_taskLaunch[] = {
    { TN("settings-enum", "Do nothing"), TaskActNothing },
    { TN("settings-enum", "Open file via local desktop environment"), TaskActFileDesk },
    { TN("settings-enum", "Open file using preferred launcher"), TaskActFile },
    { TN("settings-enum", "Open folder via local desktop environment"), TaskActDirDesk },
    { TN("settings-enum", "Open folder using preferred launcher"), TaskActDir },
    { TN("settings-enum", "Open folder in a new terminal"), TaskActTerm },
    { NULL }
};

static const ChoiceDef s_inlineAction[] = {
    { TN("settings-enum", "Do nothing"), InlineActNothing },
    { TN("settings-enum", "Open link or content"), InlineActOpen },
    { TN("settings-enum", "Select text"), InlineActSelect },
    { TN("settings-enum", "Copy text or image to clipboard"), InlineActCopy },
    { TN("settings-enum", "Write text to terminal"), InlineActWrite },
    { TN("settings-enum", "Do nothing"), InlineActNothing },
    { NULL }
};

static const ChoiceDef s_uploadConfig[] = {
    { TN("settings-enum", "Ask what to do"), Tsq::TaskAsk },
    { TN("settings-enum", "Overwrite without asking"), Tsq::TaskOverwrite },
    { TN("settings-enum", "Rename without asking"), Tsq::TaskRename },
    { TN("settings-enum", "Fail"), Tsq::TaskFail },
    { NULL }
};

static const ChoiceDef s_deleteConfig[] = {
    { TN("settings-enum", "Ask what to do"), Tsq::TaskAsk },
    { TN("settings-enum", "Ask for folders only"), Tsq::TaskAskRecurse },
    { TN("settings-enum", "Delete without asking"), Tsq::TaskOverwrite },
    { NULL }
};

static const ChoiceDef s_renameConfig[] = {
    { TN("settings-enum", "Ask what to do"), Tsq::TaskAsk },
    { TN("settings-enum", "Overwrite without asking"), Tsq::TaskOverwrite },
    { TN("settings-enum", "Fail"), Tsq::TaskFail },
    { NULL }
};

const ChoiceDef g_dropConfig[] = {
    { TN("settings-enum", "Ask what to do"), DropAsk },
    { TN("settings-enum", "Upload files to the current directory"), DropUploadCwd },
    { TN("settings-enum", "Upload files to the home directory"), DropUploadHome },
    { TN("settings-enum", "Upload files to /tmp"), DropUploadTmp },
    { TN("settings-enum", "Paste file names into the terminal"), DropPasteName },
    { TN("settings-enum", "Paste file contents into the terminal"), DropPasteContent },
    { TN("settings-enum", "Do nothing"), DropNothing },
    { NULL }
};

const ChoiceDef g_serverDropConfig[] = {
    { TN("settings-enum", "Ask what to do"), DropAsk },
    { TN("settings-enum", "Upload files to the home directory"), DropUploadHome },
    { TN("settings-enum", "Upload files to /tmp"), DropUploadTmp },
    { TN("settings-enum", "Do nothing"), DropNothing },
    { NULL }
};

const FormatDef g_termFormat[] = {
    { TN("settings-enum", "Active pane index number"), TSQT_ATTR_INDEX },
    { TN("settings-enum", "Terminal window title"), TSQ_ATTR_SESSION_TITLE },
    { TN("settings-enum", "Terminal icon name"), TSQ_ATTR_SESSION_TITLE2 },
    { TN("settings-enum", "User name"), TSQ_ATTR_SERVER_USER },
    { TN("settings-enum", "Server name"), TSQ_ATTR_SERVER_NAME },
    { TN("settings-enum", "Host name"), TSQ_ATTR_SERVER_HOST },
    { TN("settings-enum", "Command name"), TSQ_ATTR_PROC_EXE },
    { TN("settings-enum", "Command PID"), TSQ_ATTR_PROC_PID },
    { TN("settings-enum", "Newline"), "\\n" },
    { TN("settings-enum", "Literal backslash"), "\\\\" },
    { NULL }
};

const FormatDef g_serverFormat[] = {
    { TN("settings-enum", "User name"), TSQ_ATTR_SERVER_USER },
    { TN("settings-enum", "Server name"), TSQ_ATTR_SERVER_NAME },
    { TN("settings-enum", "Host name"), TSQ_ATTR_SERVER_HOST },
    { TN("settings-enum", "Terminal count"), TSQT_ATTR_COUNT },
    { TN("settings-enum", "Newline"), "\\n" },
    { TN("settings-enum", "Literal backslash"), "\\\\" },
    { NULL }
};

static const SettingDef s_globalDefs[] = {
    { "Global/LogThreshold", "logThreshold", QVariant::Int,
      TN("settings-category", "General"),
      TN("settings", "Show event log"),
      new ChoiceWidgetFactory(s_logThresholdArg)
    },
    { "Global/LogToSystem", "logToSystem", QVariant::Bool,
      TN("settings-category", "General"),
      TN("settings", "Copy event logs to the standard logging handler"),
      new CheckWidgetFactory
    },
    { "Global/RestoreGeometry", "restoreGeometry", QVariant::Bool,
      TN("settings-category", "General"),
      TN("settings", "Restore window size, position, and layout on startup"),
      new CheckWidgetFactory
    },
    { "Global/SaveGeometry", "saveGeometry", QVariant::Bool,
      TN("settings-category", "General"),
      TN("settings", "Save window size, position, and layout on exit"),
      new CheckWidgetFactory
    },
    { "Global/SaveGeometryNow", NULL, QVariant::Invalid,
      TN("settings-category", "General"),
      TN("settings", "Save window size, position, and layout now"),
      new ActionWidgetFactory("requestSaveGeometry", TN("input-button", "Save"))
    },
    { "Global/RestoreOrder", "restoreOrder", QVariant::Bool,
      TN("settings-category", "General"),
      TN("settings", "Restore order and visibility of terminals on startup"),
      new CheckWidgetFactory
    },
    { "Global/SaveOrder", "saveOrder", QVariant::Bool,
      TN("settings-category", "General"),
      TN("settings", "Save order and visibility of terminals on exit"),
      new CheckWidgetFactory
    },
    { "Global/SaveOrderNow", NULL, QVariant::Invalid,
      TN("settings-category", "General"),
      TN("settings", "Save order and visibility of terminals now"),
      new ActionWidgetFactory("requestSaveOrder", TN("input-button", "Save"))
    },
    { "Global/ProfileMenuSize", "menuSize", QVariant::Int,
      TN("settings-category", "General"),
      TN("settings", "Number of items shown in item chooser menus"),
      new IntWidgetFactory(IntWidget::Items, 0, ITEM_MENU_MAX)
    },
    { "Global/DocumentationRoot", "docRoot", QVariant::String,
      TN("settings-category", "General"),
      TN("settings", "Documentation root directory location"),
      new InputWidgetFactory
    },
    { "Global/IconTheme", "iconTheme", QVariant::String,
      TN("settings-category", "General"),
      TN("settings", "Icon theme"),
      new IconSelectFactory
    },
    { "Server/LaunchTransient", "launchTransient", QVariant::Bool,
      TN("settings-category", "Server"),
      TN("settings", "Launch a transient local server on startup"),
      new CheckWidgetFactory
    },
    { "Server/LaunchPersistent", "launchPersistent", QVariant::Bool,
      TN("settings-category", "Server"),
      TN("settings", "Connect to the persistent local server on startup"),
      new CheckWidgetFactory
    },
    { "Server/PreferTransient", "preferTransient", QVariant::Bool,
      TN("settings-category", "Server"),
      TN("settings", "Prefer transient server when creating local terminals"),
      new CheckWidgetFactory
    },
    { "Server/CloseConnectionsWindowOnLaunch", "closeOnLaunch", QVariant::Bool,
      TN("settings-category", "Server"),
      TN("settings", "Close Connections window after launching a connection"),
      new CheckWidgetFactory
    },
    { "Server/AutoQuit", "autoQuit", QVariant::Bool,
      TN("settings-category", "Server"),
      TN("settings", "Quit application after last terminal is closed"),
      new CheckWidgetFactory
    },
    { "Server/PopulateTime", "populateTime", QVariant::UInt,
      TN("settings-category", "Server"),
      TN("settings", "Time to wait for additional servers"),
      new IntWidgetFactory(IntWidget::Millis, 0, 10000, 500)
    },
    { "Server/ScrollbackFetchSpeed", "fetchSpeed", QVariant::Int,
      TN("settings-category", "Server"),
      TN("settings", "Download scrollback contents"),
      new ChoiceWidgetFactory(s_fetchSpeed)
    },
    { "Server/LocalDownloads", "localDownload", QVariant::Bool,
      TN("settings-category", "Server"),
      TN("settings", "Enable downloads and uploads to local servers"),
      new CheckWidgetFactory
    },
    { "Appearance/ShowMenuBar", "menuBar", QVariant::Bool,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Show menu bar by default"),
      new CheckWidgetFactory
    },
    { "Appearance/ShowStatusBar", "statusBar", QVariant::Bool,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Show status bar by default"),
      new CheckWidgetFactory
    },
    { "Appearance/AutoRaiseTerminals", "raiseTerminals", QVariant::Bool,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Automatically raise the terminals tool"),
      new CheckWidgetFactory
    },
    { "Appearance/ShowTerminalsPopup", "showPeek", QVariant::Bool,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Display terminals popup when switching terminals"),
      new CheckWidgetFactory
    },
    { "Appearance/TerminalsPopupTime", "peekTime", QVariant::UInt,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Display time for terminals popup"),
      new IntWidgetFactory(IntWidget::Millis, 100, 10000, 100)
    },
    { "Appearance/TerminalThumbnailCaption", "termCaption", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Format of terminal thumbnail caption"),
      new FormatWidgetFactory(g_termFormat)
    },
    { "Appearance/TerminalThumbnailTooltip", "termTooltip", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Format of terminal thumbnail tooltip"),
      new FormatWidgetFactory(g_termFormat)
    },
    { "Appearance/ServerThumbnailCaption", "serverCaption", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Format of server thumbnail caption"),
      new FormatWidgetFactory(g_serverFormat)
    },
    { "Appearance/ServerThumbnailTooltip", "serverTooltip", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Format of server thumbnail tooltip"),
      new FormatWidgetFactory(g_serverFormat)
    },
    { "Appearance/WindowTitle", "windowTitle", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Format of window title"),
      new FormatWidgetFactory(g_termFormat)
    },
    { "Appearance/ShowMainIndex", "mainIndex", QVariant::Bool,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Show active pane index in main terminal view"),
      new CheckWidgetFactory
    },
    { "Appearance/ShowThumbnailIndex", "thumbIndex", QVariant::Bool,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Show active pane index in terminal thumbnail view"),
      new CheckWidgetFactory
    },
    { "Appearance/DenseThumbnails", "denseThumb", QVariant::Bool,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Pack terminal thumbnails closely together"),
      new CheckWidgetFactory
    },
    { "Appearance/TerminalAction0", "termAction0", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Double-click action on terminal thumbnail"),
      new SlotWidgetFactory(true)
    },
    { "Appearance/TerminalAction1", "termAction1", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Control-click action on terminal thumbnail"),
      new SlotWidgetFactory(true)
    },
    { "Appearance/TerminalAction2", "termAction2", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Shift-click action on terminal thumbnail"),
      new SlotWidgetFactory(true)
    },
    { "Appearance/TerminalAction3", "termAction3", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Middle-click action on terminal thumbnail"),
      new SlotWidgetFactory(true)
    },
    { "Appearance/ServerAction0", "serverAction0", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Double-click action on server thumbnail"),
      new SlotWidgetFactory(false)
    },
    { "Appearance/ServerAction1", "serverAction1", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Control-click action on server thumbnail"),
      new SlotWidgetFactory(false)
    },
    { "Appearance/ServerAction2", "serverAction2", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Shift-click action on server thumbnail"),
      new SlotWidgetFactory(false)
    },
    { "Appearance/ServerAction3", "serverAction3", QVariant::String,
      TN("settings-category", "Appearance/Terminals Tool"),
      TN("settings", "Middle-click action on server thumbnail"),
      new SlotWidgetFactory(false)
    },
    { "Effects/MainBell", "mainBell", QVariant::Int,
      TN("settings-category", "Effects"),
      TN("settings", "Bell effect in main terminal view"),
      new ChoiceWidgetFactory(s_mainBellStyle)
    },
    { "Effects/ThumbnailBell", "thumbBell", QVariant::Int,
      TN("settings-category", "Effects"),
      TN("settings", "Bell effect in terminal thumbnail view"),
      new ChoiceWidgetFactory(s_thumbBellStyle)
    },
    { "Effects/EnableCursorBlink", "cursorBlink", QVariant::Bool,
      TN("settings-category", "Effects"),
      TN("settings", "Enable blinking cursor"),
      new CheckWidgetFactory
    },
    { "Effects/EnableTextBlink", "textBlink", QVariant::Bool,
      TN("settings-category", "Effects"),
      TN("settings", "Enable blinking text"),
      new CheckWidgetFactory
    },
    { "Effects/CursorBlinkCount", "cursorBlinks", QVariant::UInt,
      TN("settings-category", "Effects"),
      TN("settings", "Number of times to blink cursor"),
      new IntWidgetFactory(IntWidget::Blinks, 1, 1000000)
    },
    { "Effects/TextBlinkCount", "textBlinks", QVariant::UInt,
      TN("settings-category", "Effects"),
      TN("settings", "Number of times to blink text"),
      new IntWidgetFactory(IntWidget::Blinks, 1, 1000000)
    },
    { "Effects/SkipBlinkCount", "skipBlinks", QVariant::UInt,
      TN("settings-category", "Effects"),
      TN("settings", "Number of cursor blinks to skip after keyboard input"),
      new IntWidgetFactory(IntWidget::Blinks, 0, 10)
    },
    { "Effects/BlinkTime", "blinkTime", QVariant::UInt,
      TN("settings-category", "Effects"),
      TN("settings", "Duration of each half-blink"),
      new IntWidgetFactory(IntWidget::Millis, BLINK_TIME_MIN, 2000, 250)
    },
    { "Effects/EnableResizeEffect", "resizeEffect", QVariant::Bool,
      TN("settings-category", "Effects"),
      TN("settings", "Enable terminal size hint"),
      new CheckWidgetFactory
    },
    { "Effects/EnableFocusEffect", "focusEffect", QVariant::Int,
      TN("settings-category", "Effects"),
      TN("settings", "Enable terminal focus hint"),
      new ChoiceWidgetFactory(s_focusEffect)
    },
    { "Effects/EnableBadgeScrolling", "badgeEffect", QVariant::Bool,
      TN("settings-category", "Effects"),
      TN("settings", "Enable badge text scrolling"),
      new CheckWidgetFactory
    },
    { "Effects/BadgeScrollingRate", "badgeRate", QVariant::UInt,
      TN("settings-category", "Effects"),
      TN("settings", "Badge text scroll rate in milliseconds per character"),
      new IntWidgetFactory(IntWidget::Millis, BADGE_RATE_MIN, 10000, 250)
    },
    { "Command/StartInCommandMode", "commandMode", QVariant::Bool,
      TN("settings-category", "Command/Selection Mode"),
      TN("settings", "Start application in command mode"),
      new CheckWidgetFactory
    },
    { "Command/RaiseKeymapInCommandMode", "raiseCommand", QVariant::Bool,
      TN("settings-category", "Command/Selection Mode"),
      TN("settings", "Raise keymap tool on entering command mode"),
      new CheckWidgetFactory
    },
    { "Command/RaiseKeymapInSelectMode", "raiseSelect", QVariant::Bool,
      TN("settings-category", "Command/Selection Mode"),
      TN("settings", "Raise keymap tool on entering selection mode"),
      new CheckWidgetFactory
    },
    { "Command/AutoSelectMode", "autoSelect", QVariant::Bool,
      TN("settings-category", "Command/Selection Mode"),
      TN("settings", "Enter selection mode automatically"),
      new CheckWidgetFactory
    },
    { "Command/ExitSelectModeOnCopy", "copySelect", QVariant::Bool,
      TN("settings-category", "Command/Selection Mode"),
      TN("settings", "Exit selection mode on a copy action"),
      new CheckWidgetFactory
    },
    { "Command/ExitSelectModeOnWrite", "writeSelect", QVariant::Bool,
      TN("settings-category", "Command/Selection Mode"),
      TN("settings", "Exit selection mode on a write action"),
      new CheckWidgetFactory
    },
    { "Command/SelectModeInputDisabled", "inputSelect", QVariant::Bool,
      TN("settings-category", "Command/Selection Mode"),
      TN("settings", "Disable keyboard input to terminal in selection mode"),
      new CheckWidgetFactory
    },
    { "Presentation/EnterFullScreen", "presFullScreen", QVariant::Bool,
      TN("settings-category", "Presentation Mode"),
      TN("settings", "Enter full screen mode on entering presentation mode"),
      new CheckWidgetFactory
    },
    { "Presentation/HideMenuBar", "presMenuBar", QVariant::Bool,
      TN("settings-category", "Presentation Mode"),
      TN("settings", "Hide menu bar on entering presentation mode"),
      new CheckWidgetFactory
    },
    { "Presentation/HideStatusBar", "presStatusBar", QVariant::Bool,
      TN("settings-category", "Presentation Mode"),
      TN("settings", "Hide status bar on entering presentation mode"),
      new CheckWidgetFactory
    },
    { "Presentation/HideTools", "presTools", QVariant::Bool,
      TN("settings-category", "Presentation Mode"),
      TN("settings", "Hide tools on entering presentation mode"),
      new CheckWidgetFactory
    },
    { "Presentation/HideIndex", "presIndex", QVariant::Bool,
      TN("settings-category", "Presentation Mode"),
      TN("settings", "Hide active pane index in presentation mode"),
      new CheckWidgetFactory
    },
    { "Presentation/HideIndicators", "presIndicator", QVariant::Bool,
      TN("settings-category", "Presentation Mode"),
      TN("settings", "Hide ownership and mode indicators in presentation mode"),
      new CheckWidgetFactory
    },
    { "Presentation/HideBadge", "presBadge", QVariant::Bool,
      TN("settings-category", "Presentation Mode"),
      TN("settings", "Hide terminal badge in presentation mode"),
      new CheckWidgetFactory
    },
    { "Presentation/FontSizeIncrement", "presFontSize", QVariant::Int,
      TN("settings-category", "Presentation Mode"),
      TN("settings", "Increase font size by this amount in presentation mode"),
      new IntWidgetFactory(IntWidget::Points, 0, 360)
    },
    { "Inline/InlineAction0", "inlineAction0", QVariant::Int,
      TN("settings-category", "Inline Content"),
      TN("settings", "Double-click action on inline content"),
      new ChoiceWidgetFactory(s_inlineAction)
    },
    { "Inline/InlineAction1", "inlineAction1", QVariant::Int,
      TN("settings-category", "Inline Content"),
      TN("settings", "Control-click action on inline content"),
      new ChoiceWidgetFactory(s_inlineAction)
    },
    { "Inline/InlineAction2", "inlineAction2", QVariant::Int,
      TN("settings-category", "Inline Content"),
      TN("settings", "Shift-click action on inline content"),
      new ChoiceWidgetFactory(s_inlineAction)
    },
    { "Inline/DragStartMultiplier", "inlineDrag", QVariant::Int,
      TN("settings-category", "Inline Content"),
      TN("settings", "Multiplier for drag start distance on inline content"),
      new IntWidgetFactory(IntWidget::Times, 1, 10000)
    },
    { "Inline/RenderInlineImages", "renderImages", QVariant::Bool,
      TN("settings-category", "Inline Content"),
      TN("settings", "Enable rendering of inline images"),
      new CheckWidgetFactory
    },
    { "Inline/AllowSmartHyperlinks", "allowLinks", QVariant::Bool,
      TN("settings-category", "Inline Content"),
      TN("settings", "Enable remote hyperlink menus and actions"),
      new CheckWidgetFactory
    },
    { "Inline/RenderAvatars", "renderAvatars", QVariant::Bool,
      TN("settings-category", "Inline Content"),
      TN("settings", "Enable rendering of remote client avatar images"),
      new CheckWidgetFactory
    },
    { "Inline/Avatar", NULL, QVariant::Invalid,
      TN("settings-category", "Inline Content"),
      TN("settings", "Avatar image"),
      new ImageWidgetFactory(ThumbIcon::AvatarType)
    },
    { "Files/DownloadLocation", "downloadLocation", QVariant::String,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "Where to place downloaded files"),
      new DownloadWidgetFactory(0)
    },
    { "Files/DownloadFileConfirmation", "downloadConfig", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "When downloading to a file that already exists"),
      new ChoiceWidgetFactory(s_uploadConfig)
    },
    { "Files/UploadFileConfirmation", "uploadConfig", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "When uploading to a file that already exists"),
      new ChoiceWidgetFactory(s_uploadConfig)
    },
    { "Files/DeleteFileConfirmation", "deleteConfig", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "When removing a file"),
      new ChoiceWidgetFactory(s_deleteConfig)
    },
    { "Files/RenameFileConfirmation", "renameConfig", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "When renaming to a file that already exists"),
      new ChoiceWidgetFactory(s_renameConfig)
    },
    { "Files/TerminalLocalFileDrop", "termLocalFile", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "When dropping a file on a local terminal"),
      new ChoiceWidgetFactory(g_dropConfig)
    },
    { "Files/TerminalRemoteFileDrop", "termRemoteFile", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "When dropping a file on a remote terminal"),
      new ChoiceWidgetFactory(g_dropConfig)
    },
    { "Files/ThumbnailLocalFileDrop", "thumbLocalFile", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "When dropping a file on a local terminal thumbnail"),
      new ChoiceWidgetFactory(g_dropConfig)
    },
    { "Files/ThumbnailRemoteFileDrop", "thumbRemoteFile", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "When dropping a file on a remote terminal thumbnail"),
      new ChoiceWidgetFactory(g_dropConfig)
    },
    { "Files/ServerFileDrop", "serverFile", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "When dropping a file on a server thumbnail"),
      new ChoiceWidgetFactory(g_serverDropConfig)
    },
    { "Files/AutoRaiseFiles", "raiseFiles", QVariant::Bool,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "Automatically raise the files tool"),
      new CheckWidgetFactory
    },
    { "Files/FilesAction0", "fileAction0", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "Double-click action in files tool"),
      new ChoiceWidgetFactory(s_fileAction)
    },
    { "Files/FilesAction1", "fileAction1", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "Control-click action in files tool"),
      new ChoiceWidgetFactory(s_fileAction)
    },
    { "Files/FilesAction2", "fileAction2", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "Shift-click action in files tool"),
      new ChoiceWidgetFactory(s_fileAction)
    },
    { "Files/FilesAction3", "fileAction3", QVariant::Int,
      TN("settings-category", "Files/Files Tool"),
      TN("settings", "Middle-click action in files tool"),
      new ChoiceWidgetFactory(s_fileAction)
    },
    { "Tasks/AutoRaiseTasks", "raiseTasks", QVariant::Bool,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Automatically raise and lower the tasks tool"),
      new CheckWidgetFactory
    },
    { "Tasks/AutoShowTaskStatus", "autoShowTask", QVariant::Bool,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Automatically open task status dialogs"),
      new CheckWidgetFactory
    },
    { "Tasks/AutoHideTaskStatus", "autoHideTask", QVariant::Bool,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Automatically close task status dialogs"),
      new CheckWidgetFactory
    },
    { "Tasks/TasksDelayTime", "taskTime", QVariant::UInt,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Delay time before lowering the tasks tool"),
      new IntWidgetFactory(IntWidget::Millis, 0, 60000, 250)
    },
    { "Tasks/DownloadAction", "downloadAction", QVariant::Int,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Automatic action when a download task is ready"),
      new ChoiceWidgetFactory(s_taskLaunch)
    },
    { "Tasks/MountAction", "mountAction", QVariant::Int,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Automatic action when a mount task is ready"),
      new ChoiceWidgetFactory(s_taskLaunch)
    },
    { "Tasks/PreferredFileLauncher", "taskFile", QVariant::String,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Launcher for auto-opening task result files"),
      new LauncherSelectFactory
    },
    { "Tasks/PreferredDirectoryLauncher", "taskDir", QVariant::String,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Launcher for auto-opening task result directories"),
      new LauncherSelectFactory
    },
    { "Tasks/PreferredProfile", "taskProfile", QVariant::String,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Profile for auto-opening task result terminals"),
      new ProfileSelectFactory(false)
    },
    { "Tasks/TasksAction0", "taskAction0", QVariant::Int,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Double-click action in tasks tool"),
      new ChoiceWidgetFactory(s_taskAction)
    },
    { "Tasks/TasksAction1", "taskAction1", QVariant::Int,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Control-click action in tasks tool"),
      new ChoiceWidgetFactory(s_taskAction)
    },
    { "Tasks/TasksAction2", "taskAction2", QVariant::Int,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Shift-click action in tasks tool"),
      new ChoiceWidgetFactory(s_taskAction)
    },
    { "Tasks/TasksAction3", "taskAction3", QVariant::Int,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Middle-click action in tasks tool"),
      new ChoiceWidgetFactory(s_taskAction)
    },
    { "Tasks/TaskSizeLimit", "taskLimit", QVariant::Int,
      TN("settings-category", "Tasks/Tasks Tool"),
      TN("settings", "Maximum number of rows to show in tasks tool"),
      new IntWidgetFactory(IntWidget::Items, MIN_TASKVIEW_LIMIT, 1000000000)
    },
    { "Suggestions/AutoRaiseSuggestions", "raiseSuggestions", QVariant::Bool,
      TN("settings-category", "Suggestions Tool"),
      TN("settings", "Automatically raise and lower the suggestions tool"),
      new CheckWidgetFactory
    },
    { "Suggestions/SuggestionsDelayTime", "suggTime", QVariant::UInt,
      TN("settings-category", "Suggestions Tool"),
      TN("settings", "Delay time before autoraising the suggestions tool"),
      new IntWidgetFactory(IntWidget::Millis, 0, 5000, 250)
    },
    { "Suggestions/SuggestionsAction0", "suggAction0", QVariant::Int,
      TN("settings-category", "Suggestions Tool"),
      TN("settings", "Double-click action in suggestions tool"),
      new ChoiceWidgetFactory(s_suggAction)
    },
    { "Suggestions/SuggestionsAction1", "suggAction1", QVariant::Int,
      TN("settings-category", "Suggestions Tool"),
      TN("settings", "Control-click action in suggestions tool"),
      new ChoiceWidgetFactory(s_suggAction)
    },
    { "Suggestions/SuggestionsAction2", "suggAction2", QVariant::Int,
      TN("settings-category", "Suggestions Tool"),
      TN("settings", "Shift-click action in suggestions tool"),
      new ChoiceWidgetFactory(s_suggAction)
    },
    { "Suggestions/SuggestionsAction3", "suggAction3", QVariant::Int,
      TN("settings-category", "Suggestions Tool"),
      TN("settings", "Middle-click action in suggestions tool"),
      new ChoiceWidgetFactory(s_suggAction)
    },
    { "History/HistoryAction0", "jobAction0", QVariant::Int,
      TN("settings-category", "History and Notes Tools"),
      TN("settings", "Double-click action in history tool"),
      new ChoiceWidgetFactory(s_jobAction)
    },
    { "History/HistoryAction1", "jobAction1", QVariant::Int,
      TN("settings-category", "History and Notes Tools"),
      TN("settings", "Control-click action in history tool"),
      new ChoiceWidgetFactory(s_jobAction)
    },
    { "History/HistoryAction2", "jobAction2", QVariant::Int,
      TN("settings-category", "History and Notes Tools"),
      TN("settings", "Shift-click action in history tool"),
      new ChoiceWidgetFactory(s_jobAction)
    },
    { "History/HistoryAction3", "jobAction3", QVariant::Int,
      TN("settings-category", "History and Notes Tools"),
      TN("settings", "Middle-click action in history tool"),
      new ChoiceWidgetFactory(s_jobAction)
    },
    { "History/NotesAction0", "noteAction0", QVariant::Int,
      TN("settings-category", "History and Notes Tools"),
      TN("settings", "Double-click action in notes tool"),
      new ChoiceWidgetFactory(s_noteAction)
    },
    { "History/NotesAction1", "noteAction1", QVariant::Int,
      TN("settings-category", "History and Notes Tools"),
      TN("settings", "Control-click action in notes tool"),
      new ChoiceWidgetFactory(s_noteAction)
    },
    { "History/NotesAction2", "noteAction2", QVariant::Int,
      TN("settings-category", "History and Notes Tools"),
      TN("settings", "Shift-click action in notes tool"),
      new ChoiceWidgetFactory(s_noteAction)
    },
    { "History/NotesAction3", "noteAction3", QVariant::Int,
      TN("settings-category", "History and Notes Tools"),
      TN("settings", "Middle-click action in notes tool"),
      new ChoiceWidgetFactory(s_noteAction)
    },
    { "History/HistoryFont", "jobFont", QVariant::String,
      TN("settings-category", "History and Notes Tools"),
      TN("settings", "Font within history and notes tools"),
      new FontSelectFactory(false)
    },
    { "History/HistorySizeLimit", "jobLimit", QVariant::Int,
      TN("settings-category", "History and Notes Tools"),
      TN("settings", "Maximum number of rows to show in history tool"),
      new IntWidgetFactory(IntWidget::Items, 1, 1000000000)
    },
    { NULL }
};

static SettingsBase::SettingsDef s_globalDef = {
    SettingsBase::Global, s_globalDefs
};

void
GlobalSettings::populateDefaults()
{
    auto &v = s_globalDef.defaults;
    v.insert(B("logThreshold"), QtWarningMsg);
    v.insert(B("logToSystem"), true);
    v.insert(B("restoreGeometry"), true);
    v.insert(B("saveGeometry"), true);
    v.insert(B("restoreOrder"), true);
    v.insert(B("saveOrder"), true);
    v.insert(B("menuSize"), ITEM_MENU_SIZE);
    v.insert(B("docRoot"), DOCUMENTATION_ROOT);
    v.insert(B("iconTheme"), DEFAULT_ICON_THEME);
    v.insert(B("launchTransient"), true);
    v.insert(B("launchPersistent"), true);
    v.insert(B("preferTransient"), true);
    v.insert(B("closeOnLaunch"), true);
    v.insert(B("autoQuit"), true);
    v.insert(B("populateTime"), POPULATE_TIME);
    v.insert(B("fetchSpeed"), SpeedNormal);
    v.insert(B("menuBar"), true);
    v.insert(B("statusBar"), true);
    v.insert(B("raiseTerminals"), true);
    v.insert(B("showPeek"), true);
    v.insert(B("peekTime"), PEEK_TIME);
    v.insert(B("termCaption"), DEFAULT_THUMBNAIL_CAPTION);
    v.insert(B("termTooltip"), DEFAULT_THUMBNAIL_TOOLTIP);
    v.insert(B("serverCaption"), DEFAULT_SERVER_CAPTION);
    v.insert(B("serverTooltip"), DEFAULT_SERVER_TOOLTIP);
    v.insert(B("windowTitle"), DEFAULT_WINDOW_TITLE);
    v.insert(B("mainIndex"), true);
    v.insert(B("thumbIndex"), true);
    v.insert(B("termAction0"), A("RandomTerminalTheme"));
    v.insert(B("termAction1"), A("CloneTerminal"));
    v.insert(B("termAction2"), A("HideTerminal"));
    v.insert(B("termAction3"), A("PasteSelectBuffer"));
    v.insert(B("serverAction0"), A("ToggleServer"));
    v.insert(B("serverAction1"), A("NewTerminal"));
    v.insert(B("serverAction2"), A("NewWindow"));
    v.insert(B("mainBell"), BellBounce);
    v.insert(B("thumbBell"), BellFlash);
    v.insert(B("cursorBlink"), true);
    v.insert(B("textBlink"), true);
    v.insert(B("cursorBlinks"), BLINK_CURSOR_COUNT);
    v.insert(B("textBlinks"), BLINK_TEXT_COUNT);
    v.insert(B("skipBlinks"), BLINK_SKIP_COUNT);
    v.insert(B("blinkTime"), BLINK_TIME);
    v.insert(B("resizeEffect"), true);
    v.insert(B("focusEffect"), FocusSplit);
    v.insert(B("badgeEffect"), true);
    v.insert(B("badgeRate"), BADGE_RATE_DEFAULT);
    v.insert(B("raiseCommand"), true);
    v.insert(B("raiseSelect"), true);
    v.insert(B("autoSelect"), true);
    v.insert(B("presStatusBar"), true);
    v.insert(B("presTools"), true);
    v.insert(B("presIndex"), true);
    v.insert(B("inlineAction0"), InlineActOpen);
    v.insert(B("inlineAction1"), InlineActOpen);
    v.insert(B("inlineAction2"), InlineActSelect);
    v.insert(B("inlineDrag"), DEFAULT_INLINE_DRAG);
    v.insert(B("renderImages"), true);
    v.insert(B("allowLinks"), true);
    v.insert(B("renderAvatars"), true);
    v.insert(B("downloadLocation"), g_str_PROMPT_PROFILE);
    v.insert(B("downloadConfig"), Tsq::TaskAsk);
    v.insert(B("uploadConfig"), Tsq::TaskAsk);
    v.insert(B("deleteConfig"), Tsq::TaskAsk);
    v.insert(B("renameConfig"), Tsq::TaskAsk);
    v.insert(B("serverFile"), DropUploadHome);
    v.insert(B("raiseFiles"), true);
    v.insert(B("fileAction0"), FileActSmartOpen);
    v.insert(B("fileAction1"), FileActDownload);
    v.insert(B("fileAction2"), FileActCopyFile);
    v.insert(B("fileAction3"), FileActWriteFile);
    v.insert(B("raiseTasks"), true);
    v.insert(B("autoShowTask"), true);
    v.insert(B("autoHideTask"), true);
    v.insert(B("taskTime"), TASK_STATUS_TIME);
    v.insert(B("taskAction0"), TaskActInspect);
    v.insert(B("taskAction1"), TaskActFile);
    v.insert(B("taskAction2"), TaskActDir);
    v.insert(B("taskAction3"), TaskActTerm);
    v.insert(B("taskLimit"), DEFAULT_TASKVIEW_LIMIT);
    v.insert(B("raiseSuggestions"), true);
    v.insert(B("suggTime"), SUGGEST_DELAY_TIME);
    v.insert(B("suggAction0"), SuggActWrite);
    v.insert(B("suggAction1"), SuggActRemove);
    v.insert(B("suggAction2"), SuggActCopy);
    v.insert(B("suggAction3"), SuggActWriteNewline);
    v.insert(B("jobAction0"), JobActScrollStart);
    v.insert(B("jobAction1"), JobActWriteCommand);
    v.insert(B("jobAction2"), JobActSelectOutput);
    v.insert(B("jobAction3"), JobActWriteCommandNewline);
    v.insert(B("noteAction0"), NoteActScrollStart);
    v.insert(B("noteAction1"), NoteActScrollEnd);
    v.insert(B("noteAction2"), NoteActSelect);
    v.insert(B("noteAction3"), NoteActNothing);
    v.insert(B("jobFont"), L("Monospace,12"));
    v.insert(B("jobLimit"), DEFAULT_JOBVIEW_LIMIT);
}

GlobalSettings::GlobalSettings(const QString &path) :
    SettingsBase(s_globalDef, path)
{
    // initDefaults
    m_logThreshold = QtWarningMsg;
    m_menuSize = ITEM_MENU_SIZE;
    m_cursorBlinks = BLINK_CURSOR_COUNT;
    m_textBlinks = BLINK_TEXT_COUNT;
    m_skipBlinks = BLINK_SKIP_COUNT;
    m_blinkTime = BLINK_TIME;
    m_populateTime = POPULATE_TIME;
    m_taskTime = TASK_STATUS_TIME;
    m_suggTime = SUGGEST_DELAY_TIME;
    m_peekTime = PEEK_TIME;
    m_badgeRate = BADGE_RATE_DEFAULT;
    m_mainBell = BellBounce;
    m_thumbBell = BellFlash;
    m_jobLimit = DEFAULT_JOBVIEW_LIMIT;
    m_taskLimit = DEFAULT_TASKVIEW_LIMIT;
    m_fetchSpeed = SpeedNormal;
    m_focusEffect = FocusSplit;
    m_suggAction0 = SuggActWrite;
    m_suggAction1 = SuggActRemove;
    m_suggAction2 = SuggActCopy;
    m_suggAction3 = SuggActWriteNewline;
    m_jobAction0 = JobActScrollStart;
    m_jobAction1 = JobActWriteCommand;
    m_jobAction2 = JobActSelectOutput;
    m_jobAction3 = JobActWriteCommandNewline;
    m_noteAction0 = NoteActScrollStart;
    m_noteAction1 = NoteActScrollEnd;
    m_noteAction2 = NoteActSelect;
    m_noteAction3 = NoteActNothing;
    m_fileAction0 = FileActSmartOpen;
    m_fileAction1 = FileActDownload;
    m_fileAction2 = FileActCopyFile;
    m_fileAction3 = FileActWriteFile;
    m_taskAction0 = TaskActInspect;
    m_taskAction1 = TaskActFile;
    m_taskAction2 = TaskActDir;
    m_taskAction3 = TaskActTerm;
    m_inlineAction0 = InlineActOpen;
    m_inlineAction1 = InlineActSelect;
    m_inlineAction2 = InlineActOpen;
    m_inlineDrag = DEFAULT_INLINE_DRAG;
    m_uploadConfig = Tsq::TaskAsk;
    m_downloadConfig = Tsq::TaskAsk;
    m_deleteConfig = Tsq::TaskAsk;
    m_renameConfig = Tsq::TaskAsk;
    m_downloadAction = TaskActNothing;
    m_mountAction = TaskActNothing;
    m_termLocalFile = DropAsk;
    m_termRemoteFile = DropAsk;
    m_thumbLocalFile = DropAsk;
    m_thumbRemoteFile = DropAsk;
    m_serverFile = DropUploadHome;
    m_presFontSize = 0;

    m_logToSystem = true;
    m_commandMode = false;
    m_menuBar = true;
    m_statusBar = true;
    m_saveGeometry = true;
    m_restoreGeometry = true;
    m_saveOrder = true;
    m_restoreOrder = true;
    m_cursorBlink = true;
    m_textBlink = true;
    m_resizeEffect = true;
    m_badgeEffect = true;
    m_mainIndex = true;
    m_thumbIndex = true;
    m_denseThumb = false;
    m_launchTransient = true;
    m_launchPersistent = true;
    m_preferTransient = true;
    m_closeOnLaunch = true;
    m_showPeek = true;
    m_raiseTerminals = true;
    m_raiseSuggestions = true;
    m_raiseFiles = true;
    m_raiseTasks = true;
    m_autoShowTask = true;
    m_autoHideTask = true;
    m_autoQuit = true;
    m_presFullScreen = false;
    m_presMenuBar = false;
    m_presStatusBar = true;
    m_presTools = true;
    m_presIndex = true;
    m_presIndicator = false;
    m_presBadge = false;
    m_raiseSelect = true;
    m_raiseCommand = true;
    m_autoSelect = true;
    m_copySelect = false;
    m_writeSelect = false;
    m_inputSelect = false;
    m_renderImages = true;
    m_allowLinks = true;
    m_renderAvatars = true;
    m_localDownload = false;

    initColors();
}

//
// Properties
//
void
GlobalSettings::setDocRoot(QString docRoot)
{
    if (docRoot.isEmpty())
        docRoot = DOCUMENTATION_ROOT;

    REG_SETTER_BODY(docRoot)
}

void
GlobalSettings::setBlinkTime(unsigned blinkTime)
{
    if (blinkTime < BLINK_TIME_MIN)
        blinkTime = BLINK_TIME_MIN;

    NOTIFY_SETTER_BODY(blinkTime)
}

void
GlobalSettings::setJobLimit(int jobLimit)
{
    if (jobLimit <= 0)
        jobLimit = DEFAULT_JOBVIEW_LIMIT;

    REG_SETTER_BODY(jobLimit)
}

void
GlobalSettings::setTaskLimit(int taskLimit)
{
    if (taskLimit <= MIN_TASKVIEW_LIMIT)
        taskLimit = MIN_TASKVIEW_LIMIT;

    REG_SETTER_BODY(taskLimit)
}

void
GlobalSettings::setFocusEffect(int focusEffect)
{
    if (focusEffect < 0)
        focusEffect = FocusNever;
    if (focusEffect > FocusAlways)
        focusEffect = FocusAlways;

    NOTIFY_SETTER_BODY(focusEffect)
}

void
GlobalSettings::setInlineDrag(int inlineDrag)
{
    if (inlineDrag <= 0)
        inlineDrag = 1;

    REG_SETTER_BODY(inlineDrag)
}

void
GlobalSettings::setBadgeRate(unsigned badgeRate)
{
    if (badgeRate < BADGE_RATE_MIN)
        badgeRate = BADGE_RATE_MIN;

    SIG_SETTER_BODY(badgeRate, mainSettingsChanged);
}

NOTIFY_SETTER(GlobalSettings::setTermCaption, termCaption, const QString &)
NOTIFY_SETTER(GlobalSettings::setTermTooltip, termTooltip, const QString &)
NOTIFY_SETTER(GlobalSettings::setServerCaption, serverCaption, const QString &)
NOTIFY_SETTER(GlobalSettings::setServerTooltip, serverTooltip, const QString &)
NOTIFY_SETTER(GlobalSettings::setWindowTitle, windowTitle, const QString &)
NOTIFY_SETTER(GlobalSettings::setJobFont, jobFont, const QString &)
REG_SETTER(GlobalSettings::setDownloadLocation, downloadLocation, const QString &)
REG_SETTER(GlobalSettings::setTaskFile, taskFile, const QString &)
REG_SETTER(GlobalSettings::setTaskDir, taskDir, const QString &)
REG_SETTER(GlobalSettings::setTaskProfile, taskProfile, const QString &)
REG_SETTER(GlobalSettings::setTermAction0, termAction0, const QString &)
REG_SETTER(GlobalSettings::setTermAction1, termAction1, const QString &)
REG_SETTER(GlobalSettings::setTermAction2, termAction2, const QString &)
REG_SETTER(GlobalSettings::setTermAction3, termAction3, const QString &)
REG_SETTER(GlobalSettings::setServerAction0, serverAction0, const QString &)
REG_SETTER(GlobalSettings::setServerAction1, serverAction1, const QString &)
REG_SETTER(GlobalSettings::setServerAction2, serverAction2, const QString &)
REG_SETTER(GlobalSettings::setServerAction3, serverAction3, const QString &)
REG_SETTER(GlobalSettings::setLogThreshold, logThreshold, int)
SIG_SETTER(GlobalSettings::setMenuSize, menuSize, int, menuSizeChanged)
REG_SETTER(GlobalSettings::setIconTheme, iconTheme, const QString &)
NOTIFY_SETTER(GlobalSettings::setCursorBlinks, cursorBlinks, unsigned)
NOTIFY_SETTER(GlobalSettings::setTextBlinks, textBlinks, unsigned)
NOTIFY_SETTER(GlobalSettings::setSkipBlinks, skipBlinks, unsigned)
REG_SETTER(GlobalSettings::setPopulateTime, populateTime, unsigned)
REG_SETTER(GlobalSettings::setTaskTime, taskTime, unsigned)
REG_SETTER(GlobalSettings::setSuggTime, suggTime, unsigned)
REG_SETTER(GlobalSettings::setPeekTime, peekTime, unsigned)
REG_SETTER(GlobalSettings::setMainBell, mainBell, int)
REG_SETTER(GlobalSettings::setThumbBell, thumbBell, int)
NOTIFY_SETTER(GlobalSettings::setFetchSpeed, fetchSpeed, int)
ENUM_SETTER(GlobalSettings::setSuggAction0, suggAction0, SuggNAct)
ENUM_SETTER(GlobalSettings::setSuggAction1, suggAction1, SuggNAct)
ENUM_SETTER(GlobalSettings::setSuggAction2, suggAction2, SuggNAct)
ENUM_SETTER(GlobalSettings::setSuggAction3, suggAction3, SuggNAct)
ENUM_SETTER(GlobalSettings::setJobAction0, jobAction0, JobNAct)
ENUM_SETTER(GlobalSettings::setJobAction1, jobAction1, JobNAct)
ENUM_SETTER(GlobalSettings::setJobAction2, jobAction2, JobNAct)
ENUM_SETTER(GlobalSettings::setJobAction3, jobAction3, JobNAct)
ENUM_SETTER(GlobalSettings::setNoteAction0, noteAction0, NoteNAct)
ENUM_SETTER(GlobalSettings::setNoteAction1, noteAction1, NoteNAct)
ENUM_SETTER(GlobalSettings::setNoteAction2, noteAction2, NoteNAct)
ENUM_SETTER(GlobalSettings::setNoteAction3, noteAction3, NoteNAct)
ENUM_SETTER(GlobalSettings::setFileAction0, fileAction0, FileNAct)
ENUM_SETTER(GlobalSettings::setFileAction1, fileAction1, FileNAct)
ENUM_SETTER(GlobalSettings::setFileAction2, fileAction2, FileNAct)
ENUM_SETTER(GlobalSettings::setFileAction3, fileAction3, FileNAct)
ENUM_SETTER(GlobalSettings::setTaskAction0, taskAction0, TaskNAct)
ENUM_SETTER(GlobalSettings::setTaskAction1, taskAction1, TaskNAct)
ENUM_SETTER(GlobalSettings::setTaskAction2, taskAction2, TaskNAct)
ENUM_SETTER(GlobalSettings::setTaskAction3, taskAction3, TaskNAct)
ENUM_SETTER(GlobalSettings::setInlineAction0, inlineAction0, InlineNAct)
ENUM_SETTER(GlobalSettings::setInlineAction1, inlineAction1, InlineNAct)
ENUM_SETTER(GlobalSettings::setInlineAction2, inlineAction2, InlineNAct)
ENUM_SETTER(GlobalSettings::setDownloadAction, downloadAction, TaskNLaunchAct)
ENUM_SETTER(GlobalSettings::setMountAction, mountAction, TaskNLaunchAct)
REG_SETTER(GlobalSettings::setDownloadConfig, downloadConfig, int)
REG_SETTER(GlobalSettings::setUploadConfig, uploadConfig, int)
REG_SETTER(GlobalSettings::setDeleteConfig, deleteConfig, int)
REG_SETTER(GlobalSettings::setRenameConfig, renameConfig, int)
ENUM_SETTER(GlobalSettings::setTermLocalFile, termLocalFile, DropNAct)
ENUM_SETTER(GlobalSettings::setTermRemoteFile, termRemoteFile, DropNAct)
ENUM_SETTER(GlobalSettings::setThumbLocalFile, thumbLocalFile, DropNAct)
ENUM_SETTER(GlobalSettings::setThumbRemoteFile, thumbRemoteFile, DropNAct)
ENUM_SETTER(GlobalSettings::setServerFile, serverFile, DropUploadCwd)
REG_SETTER(GlobalSettings::setPresFontSize, presFontSize, int)
REG_SETTER(GlobalSettings::setLogToSystem, logToSystem, bool)
REG_SETTER(GlobalSettings::setCommandMode, commandMode, bool)
REG_SETTER(GlobalSettings::setMenuBar, menuBar, bool)
REG_SETTER(GlobalSettings::setStatusBar, statusBar, bool)
REG_SETTER(GlobalSettings::setSaveGeometry, saveGeometry, bool)
REG_SETTER(GlobalSettings::setRestoreGeometry, restoreGeometry, bool)
REG_SETTER(GlobalSettings::setSaveOrder, saveOrder, bool)
REG_SETTER(GlobalSettings::setRestoreOrder, restoreOrder, bool)
NOTIFY_SETTER(GlobalSettings::setCursorBlink, cursorBlink, bool)
NOTIFY_SETTER(GlobalSettings::setTextBlink, textBlink, bool)
NOTIFY_SETTER(GlobalSettings::setResizeEffect, resizeEffect, bool)
SIG_SETTER(GlobalSettings::setBadgeEffect, badgeEffect, bool, mainSettingsChanged)
SIG_SETTER(GlobalSettings::setMainIndex, mainIndex, bool, mainSettingsChanged)
NOTIFY_SETTER(GlobalSettings::setThumbIndex, thumbIndex, bool)
SIG_SETTER(GlobalSettings::setDenseThumb, denseThumb, bool, denseThumbChanged)
REG_SETTER(GlobalSettings::setLaunchTransient, launchTransient, bool)
REG_SETTER(GlobalSettings::setLaunchPersistent, launchPersistent, bool)
SIG_SETTER(GlobalSettings::setPreferTransient, preferTransient, bool, preferTransientChanged)
REG_SETTER(GlobalSettings::setCloseOnLaunch, closeOnLaunch, bool)
REG_SETTER(GlobalSettings::setShowPeek, showPeek, bool)
REG_SETTER(GlobalSettings::setRaiseTerminals, raiseTerminals, bool)
REG_SETTER(GlobalSettings::setRaiseSuggestions, raiseSuggestions, bool)
REG_SETTER(GlobalSettings::setRaiseFiles, raiseFiles, bool)
REG_SETTER(GlobalSettings::setRaiseTasks, raiseTasks, bool)
REG_SETTER(GlobalSettings::setAutoShowTask, autoShowTask, bool)
REG_SETTER(GlobalSettings::setAutoHideTask, autoHideTask, bool)
REG_SETTER(GlobalSettings::setAutoQuit, autoQuit, bool)
REG_SETTER(GlobalSettings::setPresFullScreen, presFullScreen, bool)
REG_SETTER(GlobalSettings::setPresMenuBar, presMenuBar, bool)
REG_SETTER(GlobalSettings::setPresStatusBar, presStatusBar, bool)
REG_SETTER(GlobalSettings::setPresTools, presTools, bool)
SIG_SETTER(GlobalSettings::setPresIndex, presIndex, bool, mainSettingsChanged)
SIG_SETTER(GlobalSettings::setPresIndicator, presIndicator, bool, mainSettingsChanged)
SIG_SETTER(GlobalSettings::setPresBadge, presBadge, bool, mainSettingsChanged)
REG_SETTER(GlobalSettings::setRaiseSelect, raiseSelect, bool)
REG_SETTER(GlobalSettings::setRaiseCommand, raiseCommand, bool)
REG_SETTER(GlobalSettings::setAutoSelect, autoSelect, bool)
REG_SETTER(GlobalSettings::setCopySelect, copySelect, bool)
REG_SETTER(GlobalSettings::setWriteSelect, writeSelect, bool)
REG_SETTER(GlobalSettings::setInputSelect, inputSelect, bool)
REG_SETTER(GlobalSettings::setRenderImages, renderImages, bool)
REG_SETTER(GlobalSettings::setAllowLinks, allowLinks, bool)
REG_SETTER(GlobalSettings::setRenderAvatars, renderAvatars, bool)
REG_SETTER(GlobalSettings::setLocalDownload, localDownload, bool)

void
GlobalSettings::requestSaveGeometry()
{
    emit actionSaveGeometry();
}

void
GlobalSettings::requestSaveOrder()
{
    emit actionSaveOrder();
}

//
// Colors
//
void
GlobalSettings::initColors()
{
    m_colors[MinorBg] = Qt::yellow;
    m_colors[MinorFg] = Qt::black;
    m_colors[MajorBg] = Qt::red;
    m_colors[MajorFg] = Qt::black;

    m_colors[StartFg] = qRgb(255, 255, 0);
    m_colors[ErrorFg] = qRgb(221, 0, 0);
    m_colors[ConnFg] = m_colors[FinishFg] = qRgb(0, 204, 0);
    m_colors[DisconnFg] = m_colors[CancelFg] = qRgb(255, 176, 0);

    m_colors[BellFg] = Qt::red;
}

//
// Documentation links
//
QString
GlobalSettings::docUrl(const QString &page) const
{
    return m_docRoot + '/' + page;
}

QString
GlobalSettings::docLink(const char *page, const char *text) const
{
    return L("<a href='%1/%2'>%3</a>").arg(m_docRoot, page, text);
}
