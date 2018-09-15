// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "order.h"
#include "url.h"

QT_BEGIN_NAMESPACE
class QMimeData;
QT_END_NAMESPACE
class MainWindow;
class TermScrollport;
class TermStack;
class TermKeymap;
class TermTask;
struct TermShortcut;
struct TermSig;
class ProfileSettings;
class LaunchSettings;

enum ClipboardCopyType {
    CopiedChars, CopiedBytes, CopiedImage
};

class TermManager final: public TermOrder
{
    Q_OBJECT

    friend class TermListener;

private:
    static QHash<QByteArray,int> s_actions;
    static QStringList s_actionSignatures;
    static QStringList s_termSignatures;
    static QStringList s_serverSignatures;

private:
    MainWindow *m_parent = nullptr;

    ServerInstance *m_server;
    TermInstance *m_term = nullptr;
    TermScrollport *m_scrollport = nullptr;

    QList<TermStack*> m_stacks;
    TermStack *m_stack = nullptr;

    QString m_title;
    QMetaObject::Connection m_mocCombo, m_mocTitle;

    void setActiveServer(ServerInstance *server);
    TermInstance* lookupTerm(const QString &terminalId);
    ServerInstance* lookupServer(const QString &serverId);
    TermTask* lookupTask(const QString &index);
    bool lookupJob(const QString &regionId, const QString &terminalId, regionid_t &idret);
    QString lookupCommand(const QString &regionId, const QString &terminalId);
    regionid_t lastToolRegion();

    void raiseTerminalsWidget(bool peek);
    bool raiseAdjustDialog(TermInstance *term, const QMetaObject &metaObj);

    void performFileDrop(TermInstance *term, ServerInstance *server,
                         const QMimeData *data, int action);

    bool checkRemotePath(ServerInstance *server, const TermUrl &tu, const QString &path);
    void postOpen(ServerInstance *server, LaunchSettings *launcher,
                  const QUrl &url, const AttributeMap &subs);
    void postOpenFile(ServerInstance *server, LaunchSettings *launcher,
                      const TermUrl &tu, AttributeMap subs);

    // Interactive action follow-ups
    void postRenameFile(ServerInstance *server, QString oldPath, QString newPath);
    void postDownloadImage(TermInstance *term, QString contentId,
                           QString localPath, bool overwrite);
    void postDownloadFile(ServerInstance *server, QString remotePath,
                          QString localPath, bool overwrite);
    void preSave(QObject *obj, int type, QString localPath);
    void postSave(QObject *obj, int type, QString localPath, bool overwrite);

signals:
    void invoking(const QString &slot);
    void invokeRequest(QString slot, bool fromKeyboard);

    void activeChanged(bool active);

    void titleChanged(const QString &title);

    void serverActivated(ServerInstance *server);
    void termActivated(TermInstance *term, TermScrollport *scrollport);

    void stackReordered();
    void menuFinished();

    void populated();
    void finished();

private slots:
    void timerCallback();

    void handleServerAdded(ServerInstance *server);
    void handleServerRemoved(ServerInstance *server);

    void handleTermAdded(TermInstance *term);
    void handleTermRemoved(TermInstance *term);

    void handleTitle(const QString &title);
    void handleStackDestroyed(QObject *object);

    void handleComboFinished(int code);

public:
    TermManager();

    inline MainWindow* parent() { return m_parent; }
    inline QWidget* parentWidget() { return (QWidget*)m_parent; }
    void setParent(MainWindow *parent);

    inline bool active() const { return m_active; }
    inline bool populating() const { return m_populating; }
    inline ServerInstance* activeServer() { return m_server; }
    inline TermInstance* activeTerm() { return m_term; }
    inline TermScrollport* activeScrollport() { return m_scrollport; }
    inline TermStack* activeStack() { return m_stack; }
    inline const auto& stacks() const { return m_stacks; }
    inline const QString& title() const { return m_title; }

    // Used by listener
    TermInstance* createTerm(ServerInstance *server, ProfileSettings *profile,
                             TermSig *copyfrom = nullptr,
                             bool duplicate = false);
    void launchTerm(ServerInstance *server, const LaunchSettings *launcher,
                    const AttributeMap &subs);
    void raiseTerm(TermInstance *term);

    // Used by menus
    TermInstance* lastToolTerm();
    ServerInstance* lastToolServer();

    void setActive(bool active);
    void setActiveTerm(TermInstance *term, TermScrollport *scrollport);
    void setActiveStack(TermStack *stack);
    void addStack(TermStack *stack, int pos);

    void saveOrder(OrderLayoutWriter &layout) const;
    void loadOrder(OrderLayout &layout);

    // File drops
    void terminalFileDrop(TermInstance *term, const QMimeData *data);
    void thumbnailFileDrop(TermInstance *term, const QMimeData *data);
    void serverFileDrop(ServerInstance *server, const QMimeData *data);
    void reportClipboardCopy(int size, int type = CopiedChars,
                             bool selbuf = false, bool remote = false);

    // Meta functions
    static void initialize();
    void invokeSlot(const QString &slot, bool fromKeyboard = false);
    static bool validateSlot(const QByteArray &candidate);
    bool lookupShortcut(QString &slot, QString &keymap, TermShortcut *result);
    void reportComboStarted(const TermKeymap *keymap);
    static inline const auto& allSlots() { return s_actionSignatures; }
    static inline const auto& termSlots() { return s_termSignatures; }
    static inline const auto& serverSlots() { return s_serverSignatures; }

public slots:
    //
    // File actions
    //
    void actionNewWindow(QString profileName, QString serverId);
    void actionNewTerminal(QString profileName, QString serverId);
    void actionNewLocalTerminal(QString profileName);
    void actionCommandTerminal(QString profileName, QString serverId, QString cmdspec);
    void actionCloneTerminal(QString duplicate, QString terminalId);
    void actionCloseTerminal(QString terminalId);
    void actionCloseWindow();
    void actionQuitApplication();

    void actionSaveAll(QString localPath, QString terminalId);
    void actionSaveScreen(QString localPath, QString terminalId);

    //
    // Edit actions
    //
    void actionCopy(QString terminalId);
    void actionCopyAll(QString terminalId);
    void actionCopyScreen(QString format, QString terminalId);
    void actionPaste(QString terminalId);
    void actionPasteSelectBuffer(QString terminalId);
    void actionPasteFile(QString terminalId, QString filePath);
    void actionWriteText(QString text, QString terminalId);
    void actionWriteTextNewline(QString text, QString terminalId);

    void actionFind();
    void actionSearchUp();
    void actionSearchDown();
    void actionSearchReset();

    void actionSelectAll();
    void actionSelectScreen();
    void actionSelectLine(QString arg);
    void actionSelectHandle(QString arg);
    void actionSelectHandleForwardChar(QString arg);
    void actionSelectHandleBackChar(QString arg);
    void actionSelectHandleForwardWord(QString arg);
    void actionSelectHandleBackWord(QString arg);
    void actionSelectHandleDownLine(QString arg);
    void actionSelectHandleUpLine(QString arg);
    void actionSelectMoveDownLine();
    void actionSelectMoveUpLine();
    void actionSelectMoveForwardWord();
    void actionSelectMoveBackWord();
    void actionSelectWord(QString index);
    void actionWriteSelection();
    void actionWriteSelectionNewline();

    void actionAnnotateSelection();
    void actionAnnotateScreen(QString row);
    void actionAnnotateRegion(QString regionId, QString terminalId);
    void actionAnnotateOutput(QString regionId, QString terminalId);
    void actionAnnotateCommand(QString regionId, QString terminalId);
    void actionAnnotateLineContext();
    void actionAnnotateOutputContext();
    void actionAnnotateCommandContext();
    void actionRemoveNote(QString regionId, QString terminalId);

    void actionWriteSuggestion(QString index);
    void actionWriteSuggestionNewline(QString index);
    void actionCopySuggestion(QString index);
    void actionRemoveSuggestion(QString index);
    void actionSuggestFirst();
    void actionSuggestPrevious();
    void actionSuggestNext();
    void actionSuggestLast();

    void actionToggleCommandMode(QString arg);
    void actionToggleSelectionMode(QString arg);

    void actionCopyRegion(QString regionId, QString terminalId);
    void actionCopySemantic(QString regionId, QString terminalId);
    void actionCopyUrl(QString url);
    void actionOpenDesktopUrl(QString url);
    void actionSetSelectedUrl(QString url);

    //
    // View actions
    //
    void actionNextPane();
    void actionPreviousPane();
    void actionSwitchPane(QString paneIndex);

    void actionSplitViewHorizontal();
    void actionSplitViewHorizontalFixed();
    void actionSplitViewVertical();
    void actionSplitViewVerticalFixed();
    void actionSplitViewQuadFixed();
    void actionSplitViewClose();
    void actionSplitViewCloseOthers();
    void actionSplitViewExpand();
    void actionSplitViewShrink();
    void actionSplitViewEqualize();
    void actionSplitViewEqualizeAll();

    void actionScrollLineUp();
    void actionScrollPageUp();
    void actionScrollToTop();
    void actionScrollLineDown();
    void actionScrollPageDown();
    void actionScrollToBottom();

    void actionScrollRegionStart(QString regionId, QString terminalId);
    void actionScrollRegionEnd(QString regionId, QString terminalId);
    void actionScrollRegionRelative(QString regionId, QString offset);
    void actionScrollSemantic(QString regionId);
    void actionScrollSemanticRelative(QString regionId, QString offset);
    void actionScrollImage(QString terminalId, QString contentId);
    void actionHighlightCursor();
    void actionHighlightSemanticRegions();

    void actionToggleFullScreen();
    void actionExitFullScreen();
    void actionTogglePresentationMode();
    void actionExitPresentationMode();
    void actionToggleMenuBar();
    void actionShowMenuBar();
    void actionToggleStatusBar();
    void actionTerminalContextMenu();

    void actionToggleTerminalFollowing();
    void actionToggleTerminalLayout(QString item, QString terminalId);
    void actionAdjustTerminalLayout(QString terminalId);
    void actionAdjustTerminalFont(QString terminalId);
    void actionAdjustTerminalColors(QString terminalId);
    void actionRandomTerminalTheme(QString sameGroup, QString terminalId);
    void actionIncreaseFont();
    void actionDecreaseFont();
    void actionUndoTerminalAdjustments(QString terminalId);
    void actionUndoAllAdjustments();

    //
    // Dock actions
    //
    void actionToggleTerminalsTool();
    void actionToggleKeymapTool();
    void actionToggleSuggestionsTool();
    void actionToggleSearchTool();
    void actionToggleHistoryTool();
    void actionToggleAnnotationsTool();
    void actionToggleTasksTool();
    void actionToggleFilesTool();
    void actionRaiseTerminalsTool();
    void actionRaiseKeymapTool();
    void actionRaiseSuggestionsTool();
    void actionRaiseSearchTool();
    void actionRaiseHistoryTool();
    void actionRaiseAnnotationsTool();
    void actionRaiseTasksTool();
    void actionRaiseFilesTool();
    void actionRaiseActiveTool();

    void actionHistoryFirst();
    void actionHistoryPrevious();
    void actionHistoryNext();
    void actionHistoryLast();
    void actionHistorySearch();
    void actionHistorySearchReset();

    void actionNoteFirst();
    void actionNotePrevious();
    void actionNoteNext();
    void actionNoteLast();
    void actionNoteSearch();
    void actionNoteSearchReset();

    void actionTaskFirst();
    void actionTaskPrevious();
    void actionTaskNext();
    void actionTaskLast();

    void actionFileFirst();
    void actionFilePrevious();
    void actionFileNext();
    void actionFileLast();
    void actionFileSearch();
    void actionFileSearchReset();

    void actionToolFirst();
    void actionToolPrevious();
    void actionToolNext();
    void actionToolLast();
    void actionToolSearch();
    void actionToolSearchReset();
    void actionToolAction(QString index);
    void actionToolContextMenu();
    void actionToggleToolSearchBar();
    void actionToggleToolTableHeader();

    //
    // Terminal actions
    //
    void actionFirstTerminal();
    void actionPreviousTerminal();
    void actionNextTerminal();
    void actionSwitchTerminal(QString terminalId, QString paneIndex);
    void actionHideTerminal(QString terminalId);
    void actionHideTerminalEverywhere(QString terminalId);
    void actionShowTerminal(QString terminalId);
    void actionReorderTerminalForward(QString terminalId);
    void actionReorderTerminalBackward(QString terminalId);
    void actionReorderTerminalFirst(QString terminalId);
    void actionReorderTerminalLast(QString terminalId);

    void actionClearTerminalScrollback(QString terminalId);
    void actionAdjustTerminalScrollback(QString terminalId);
    void actionResetTerminal(QString terminalId);
    void actionResetAndClearTerminal(QString terminalId);

    void actionSendSignal(QString signal, QString terminalId);
    void actionTakeTerminalOwnership(QString terminalId);
    void actionToggleTerminalRemoteInput(QString terminalId);
    void actionToggleSoftScrollLock(QString terminalId);

    void actionTimingSetOriginContext();
    void actionTimingFloatOrigin();

    void actionInputSetLeader(QString terminalId);
    void actionInputUnsetLeader();
    void actionInputSetFollower(QString terminalId);
    void actionInputUnsetFollower(QString terminalId);
    void actionInputToggleFollower(QString terminalId);

    //
    // Server actions
    //
    void actionPreviousServer();
    void actionNextServer();
    void actionSwitchServer(QString serverId, QString paneIndex);
    void actionHideServer(QString serverId);
    void actionShowServer(QString serverId);
    void actionToggleServer(QString serverId);
    void actionReorderServerForward(QString serverId);
    void actionReorderServerBackward(QString serverId);
    void actionReorderServerFirst(QString serverId);
    void actionReorderServerLast(QString serverId);

    void actionSetTerminalIcon(QString icon, QString terminalId);
    void actionSetServerIcon(QString icon, QString serverId);
    void actionViewTerminalInfo(QString terminalId);
    void actionViewTerminalContent(QString contentId, QString terminalId);
    void actionViewServerInfo(QString serverId);
    void actionDisconnectTerminal(QString terminalId);
    void actionDisconnectServer(QString serverId);

    void actionOpenConnection(QString connName);
    void actionNewConnection(QString connType, QString connArg, QString serverId);

    void actionLocalPortForward(QString serverId, QString spec);
    void actionRemotePortForward(QString serverId, QString spec);

    void actionSendMonitorInput(QString serverId, QString message);

    //
    // Job actions
    //
    void actionScrollPromptFirst();
    void actionScrollPromptUp();
    void actionScrollPromptDown();
    void actionScrollPromptLast();
    void actionScrollNoteUp();
    void actionScrollNoteDown();
    void actionScrollJobStartContext();
    void actionScrollJobEndContext();

    void actionWriteCommand(QString regionId, QString terminalId);
    void actionWriteCommandNewline(QString regionId, QString terminalId);
    void actionCopyCommand(QString regionId, QString terminalId);
    void actionCopyOutput(QString regionId, QString terminalId);
    void actionCopyJob(QString regionId, QString terminalId);
    void actionCopyCommandContext();
    void actionCopyOutputContext();
    void actionCopyJobContext();
    void actionSelectCommand(QString regionId, QString terminalId);
    void actionSelectOutput(QString regionId, QString terminalId);
    void actionSelectJob(QString regionId, QString terminalId);
    void actionSelectCommandContext();
    void actionSelectOutputContext();
    void actionSelectJobContext();

    void actionToolFilterRemoveClosed();
    void actionToolFilterAddTerminal(QString terminalId);
    void actionToolFilterSetTerminal(QString terminalId);
    void actionToolFilterAddServer(QString serverId);
    void actionToolFilterSetServer(QString serverId);
    void actionToolFilterExcludeTerminal(QString terminalId);
    void actionToolFilterExcludeServer(QString serverId);
    void actionToolFilterIncludeNothing();
    void actionToolFilterReset();

    void actionOpenUrl(QString launcherName, QString serverId, QString url);
    void actionOpenFile(QString launcherName, QString serverId, QString remotePath,
                        QString substitutions);
    void actionDownloadFile(QString serverId, QString remotePath, QString localPath);
    void actionUploadFile(QString serverId, QString remotePath, QString localPath);
    void actionUploadToDirectory(QString serverId, QString remoteDir, QString localPath);
    void actionRenameFile(QString serverId, QString remotePath, QString newPath);
    void actionDeleteFile(QString serverId, QString remotePath);
    void actionMountFile(QString readonly, QString serverId, QString remotePath);
    void actionDownloadImage(QString terminalId, QString contentId, QString localPath);
    void actionFetchImage(QString terminalId, QString contentId);
    void actionCopyFile(QString format, QString serverId, QString remotePath);
    void actionCopyImage(QString format, QString terminalId, QString contentId);

    void actionRunCommand(QString serverId, QString cmdspec, QString startdir);
    void actionPopupCommand(QString serverId, QString cmdspec, QString startdir);
    void actionLaunchCommand(QString launcherName, QString serverId,
                             QString substitutions);

    void actionCopyFilePath(QString filePath);
    void actionCopyDirectoryPath(QString dirPath);
    void actionWriteFilePath(QString filePath);
    void actionWriteDirectoryPath(QString dirPath);
    void actionSetFileListingFormat(QString format);
    void actionToggleFileListingFormat();
    void actionSetFileListingSort(QString spec);

    void actionOpenTaskFile(QString launcherName, QString taskId);
    void actionOpenTaskDirectory(QString launcherName, QString taskId);
    void actionOpenTaskTerminal(QString profileName, QString taskId);
    void actionCancelTask(QString taskId);
    void actionRestartTask(QString taskId);
    void actionInspectTask(QString taskId);
    void actionRemoveTasks();
    void actionCopyTaskFile(QString format, QString taskId);
    void actionCopyTaskFilePath(QString taskId);
    void actionCopyTaskDirectoryPath(QString taskId);
    void actionWriteTaskFilePath(QString taskId);
    void actionWriteTaskDirectoryPath(QString taskId);

    //
    // Settings actions
    //
    void actionEditProfile(QString profileName);
    void actionEditKeymap(QString keymapName);
    void actionEditServer(QString serverId);
    void actionSwitchProfile(QString profileName, QString terminalId);
    void actionPushProfile(QString profileName, QString terminalId);
    void actionPopProfile(QString terminalId);
    void actionExtractProfile(QString terminalId);

    void actionSetAlert(QString alertName, QString terminalId);
    void actionClearAlert(QString terminalId);

    void actionManageKeymaps();
    void actionManageProfiles();
    void actionManageLaunchers();
    void actionManageConnections();
    void actionManageServers();
    void actionManageAlerts();
    void actionManagePlugins();
    void actionManagePortForwarding();
    void actionManageTerminals();

    void actionEditGlobalSettings();
    void actionEditSwitchRules();
    void actionEditIconRules();

    void actionPrompt();
    void actionNotifySend(QString summary, QString body);
    void actionEventLog();
    void actionManpageTerminal(QString manpage);
    void actionTipOfTheDay();
    void actionHelpAbout();
};

#define COMBO_BEGIN 0
#define COMBO_SUCCESS 1
#define COMBO_FAIL 2
#define COMBO_CANCEL 3
#define COMBO_RESET 4

#define ACTION_PREFIX "action"
#define ACTION_PREFIX_LEN (sizeof(ACTION_PREFIX) - 1)
#define CUSTOM_PREFIX "Custom"
#define CUSTOM_PREFIX_LEN (sizeof(CUSTOM_PREFIX) - 1)
