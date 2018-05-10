// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QMenu>

QT_BEGIN_NAMESPACE
class QSignalMapper;
QT_END_NAMESPACE
class TermManager;
class TermInstance;
class ServerInstance;
class MainWindow;

// Dynamic Menu Features
enum MenuFeature {
    NoMenuFeature,
    ServerTermMenu, LocalTermMenu, NewTermMenu, NewWindowMenu, NewConnMenu,
    SettingsMenu, TermMenu, ConnectMenu, LayoutMenu, EditMenu, InputMenu,
    ServerMenu, ToolsMenu, OpenFileMenu, OpenTaskMenu, AlertMenu,
    SwitchProfileMenu, PushProfileMenu, TermIconMenu, ServerIconMenu
};

// Dynamic Action Features
enum MenuActionFeature : uint64_t {
    DynCheckable                = (uint64_t)1 << 63,
    DynAutoHide                 = (uint64_t)1 << 62,
    DynSuggestionsTool          = DynAutoHide|(uint64_t)1 << 61,
    DynSearchTool               = DynAutoHide|(uint64_t)1 << 60,
    DynHistoryTool              = DynAutoHide|(uint64_t)1 << 59,
    DynNotesTool                = DynAutoHide|(uint64_t)1 << 58,
    DynFilesTool                = DynAutoHide|(uint64_t)1 << 57,
    DynTasksTool                = DynAutoHide|(uint64_t)1 << 56,
    DynToolMask                 = (uint64_t)0x3f << 56,
    // Not enabled = 0
    // Enabled = 1
    DynEditProfile              = (uint64_t)1 << 1,
    DynEditKeymap               = (uint64_t)1 << 2,
    DynEditServer               = DynAutoHide|(uint64_t)1 << 3,
    DynNewTerminal              = DynAutoHide|(uint64_t)1 << 4,
    DynNotOurTerm               = (uint64_t)1 << 5,
    DynRestoreMenuBar           = DynAutoHide|(uint64_t)1 << 6,
    DynExitFullScreenMode       = DynAutoHide|(uint64_t)1 << 7,
    DynExitPresentationMode     = DynAutoHide|(uint64_t)1 << 8,
    DynFollowable               = DynCheckable|(uint64_t)1 << 9,
    DynInputtable               = DynCheckable|(uint64_t)1 << 10,
    DynHasPeer                  = (uint64_t)1 << 11,
    DynTransientConn            = (uint64_t)1 << 12,
    DynPersistentConn           = (uint64_t)1 << 13,
    DynHaveSelection            = (uint64_t)1 << 14,
    DynLayoutItem1              = DynCheckable|(uint64_t)1 << 15,
    DynLayoutItem2              = DynCheckable|(uint64_t)1 << 16,
    DynLayoutItem3              = DynCheckable|(uint64_t)1 << 17,
    DynLayoutItem4              = DynCheckable|(uint64_t)1 << 18,
    DynHaveLastToolTerm         = (uint64_t)1 << 19,
    DynHaveLastToolServer       = (uint64_t)1 << 20,
    DynLastToolName             = DynAutoHide|(uint64_t)1 << 21,
    DynLastToolIsSearchable     = DynAutoHide|(uint64_t)1 << 22,
    DynLastToolIsFilterable     = DynAutoHide|(uint64_t)1 << 23,
    DynLastToolTableHeaderCheck = DynAutoHide|DynCheckable|(uint64_t)1 << 24,
    DynLastToolSearchBarCheck   = DynAutoHide|DynCheckable|(uint64_t)1 << 25,
    DynLastToolDisplay          = DynAutoHide|(uint64_t)1 << 26,
    DynHistoryToolAndTerm       = DynHistoryTool|(uint64_t)1 << 27,
    DynNotesToolAndTerm         = DynNotesTool|(uint64_t)1 << 28,
    DynTasksToolAndTask         = DynTasksTool|(uint64_t)1 << 29,
    DynTasksToolAndCancel       = DynTasksTool|(uint64_t)1 << 30,
    DynTasksToolAndClone        = DynTasksTool|(uint64_t)1 << 31,
    DynTasksToolAndFileHide     = DynTasksTool|(uint64_t)1 << 32,
    DynTasksToolOpenFile        = DynTasksTool|(uint64_t)1 << 33,
    DynFilesToolAndDir          = DynFilesTool|(uint64_t)1 << 34,
    DynFilesToolAndFile         = DynFilesTool|(uint64_t)1 << 35,
    DynFilesToolAndFileHide     = DynFilesTool|(uint64_t)1 << 36,
    DynFilesToolAndRemoteDir    = DynFilesTool|(uint64_t)1 << 37,
    DynFilesToolAndRemoteFile   = DynFilesTool|(uint64_t)1 << 38,
    DynFilesToolFormatShort     = DynFilesTool|(uint64_t)1 << 39,
    DynFilesToolFormatLong      = DynFilesTool|(uint64_t)1 << 40,
    DynFilesToolMountableFile   = (uint64_t)1 << 41,
    DynFilesToolOpenFile        = DynFilesTool|(uint64_t)1 << 42,
    DynLaunchItem               = (uint64_t)1 << 43,
    DynConnItem                 = DynAutoHide|(uint64_t)1 << 44,
    DynClearAlert               = DynAutoHide|(uint64_t)1 << 45,
    DynInputLeader              = (uint64_t)1 << 46,
    DynInputFollower            = DynCheckable|(uint64_t)1 << 47,
    DynInputMultiplexing        = (uint64_t)1 << 48,
};

#define OBJPROP_FLAG "dynFlag"
#define setDynFlag(x) setProperty(OBJPROP_FLAG, (qulonglong)x)

class DynamicMenu final: public QMenu
{
    Q_OBJECT

private:
    MainWindow *m_window;
    QSignalMapper *m_slotMapper;
    const char *m_id;

    TermManager *m_manager;
    TermInstance *m_term;
    ServerInstance *m_server;

    int m_feature = NoMenuFeature;

    void setup(const char *id, MainWindow *window);
    int adjustLaunchItems(const QString &path, const QString &slot,
                          const QList<QAction*> &actions);

private slots:
    void handleTermActivated(TermInstance *term);
    void handleServerActivated(ServerInstance *server);
    void handleConfirmation();

    void preshowAlerts();
    void preshowEditSettings();
    void preshowSwitchProfile();
    void preshowTermMenu();
    void preshowConnectMenu();
    void preshowEditMenu();
    void preshowInputMenu();
    void preshowLayoutMenu();
    void preshowServerMenu();
    void preshowToolsMenu();
    void preshowOpenFileMenu();
    void preshowOpenTaskMenu();
    void preshowServerTermMenu();
    void preshow();

    void updateLocalTerm();
    void updateNewTerm();
    void updateNewWindow();
    void updateNewConn();
    void updateAlerts();
    void updateSwitchProfile();
    void updatePushProfile();
    void updateTermIcons();
    void updateServerIcons();
    void updateToolsMenu();
    void updateLaunchMenu();
    void updateServerTermMenu();

public slots:
    void updateConnectMenu();

protected:
    bool event(QEvent *event);

public:
    DynamicMenu(const char *id, MainWindow *window, QWidget *parent);
    DynamicMenu(TermInstance *term, const char *id,
                MainWindow *window, QWidget *parent);
    DynamicMenu(ServerInstance *server, const char *id,
                MainWindow *window, QWidget *parent);

    void enable(MenuFeature feature);
    void enable(QAction *a, uint64_t flag);
    void setConfirmation(QAction *a, const QString &message);

    QAction* addAction();
    QAction* addAction(const QString &text, const QString &tooltip,
                       const QIcon &icon, const QString &slot,
                       uint64_t flag = 1);
    QAction* addAction(const QString &text, const QString &tooltip,
                       const std::string &icon, const QString &slot);
    QAction* addAction(const char *text, const char *tooltip,
                       const std::string &icon, const QString &slot,
                       uint64_t flag = 1);
    QAction* addAction(const QString &text);
    void adjustAction(QAction *action,
                      const QString &text, const QString &tooltip,
                      const QIcon &icon, const QString &slot);

    QAction* addMenu(QMenu *subMenu, const char *text);
    QAction* addMenu(QMenu *subMenu, const char *text, const std::string &icon);
};

inline QAction *
DynamicMenu::addAction(const QString &text)
{
    return QMenu::addAction(text);
}
