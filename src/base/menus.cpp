// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/actions.h"
#include "app/attr.h"
#include "app/icons.h"
#include "mainwindow.h"
#include "menubase.h"
#include "menumacros.h"
#include "server.h"
#include "term.h"
#include "filewidget.h"
#include "settings/settings.h"

#include <QMenuBar>

void
MainWindow::createMenus()
{
    QMenuBar *bar = menuBar();
    DynamicMenu *menu, *subMenu;

    /* File menu */
    MENU_S("menu-file", ServerMenu, TL("menu-main", "File"));
    MI(TN("menu-file", "&New Terminal on %1"), ACT_NEW_TERMINAL, ICON_NEW_TERMINAL, L("NewTerminal"), DynNewTerminal);

    SUBMENU_S(0, "dmenu-newterm", NewTermMenu, TN("menu-file", "New &Terminal with Profile"));

    MS();

    SUBMENU_E("dmenu-newtermon", ServerTermMenu, TN("menu-file", "New Terminal &on Server"), ICON_NEW_TERMINAL_SERVER);

    MI(TN("menu-edit-input", "Manage Terminals") "...", ACT_MANAGE_TERMINALS, ICON_MANAGE_TERMINALS, L("ManageTerminals"));
    MS();
    MI(TN("menu-file", "New &Local Terminal"), ACT_NEW_LOCAL_TERMINAL, ICON_NEW_TERMINAL_LOCAL, L("NewLocalTerminal"));

    SUBMENU_E("dmenu-newterm", LocalTermMenu, TN("menu-file", "New Local Terminal with &Profile"));

    MI(TN("menu-file", "New &Window"), ACT_NEW_WINDOW, ICON_NEW_WINDOW, L("NewWindow"));
    MS();
    MI(TN("menu-file", "Save &All As") "...", ACT_SAVE_ALL, ICON_SAVE_ALL, L("SaveAll"));
    MI(TN("menu-file", "Save &Screen As") "...", ACT_SAVE_SCREEN, ICON_SAVE_SCREEN, L("SaveScreen"));
    MS();
    MI(TN("menu-file", "&Close Window"), ACT_CLOSE_WINDOW, ICON_CLOSE_WINDOW, L("CloseWindow"));
    MI(TN("menu-file", "&Quit Application"), ACT_QUIT_APPLICATION, ICON_QUIT_APPLICATION, L("QuitApplication"));

    /* Edit menu */
    MENU_E("menu-edit", EditMenu, TL("menu-main", "Edit"));
    MI(TN("menu-edit", "&Copy"), ACT_COPY, ICON_COPY, L("Copy"));
    MI(TN("menu-edit", "&Paste"), ACT_PASTE, ICON_PASTE, L("Paste"));
    MI(TN("menu-edit", "&Paste from File") "...", ACT_PASTE_FILE, ICON_PASTE_FILE, L("PasteFile"));
    MI(TN("menu-edit", "&Find"), ACT_FIND, ICON_SEARCH, L("Find"));
    MI(TN("menu-edit", "&Reset Search"), ACT_SEARCH_RESET, ICON_RESET_SEARCH, L("SearchReset"));
    MS();
    MI(TN("menu-edit", "Copy &Screen"), ACT_COPY_SCREEN, ICON_COPY_SCREEN, L("CopyScreen"));

    SUBMENU("menu-edit-copy", TN("menu-edit", "Copy"), ICON_COPY);
    SI(TN("menu-edit-copy", "&All"), ACT_COPY_ALL, ICON_COPY_ALL, L("CopyAll"));
    SI(TN("menu-edit-copy", "Screen as &Image"), ACT_COPY_SCREEN, ICON_COPY_IMAGE, L("CopyScreen|png"));
    SI(TN("menu-edit-copy", "Screen as &Text"), ACT_COPY_SCREEN, ICON_COPY_SCREEN, L("CopyScreen"));
    SS();
    SI(TN("menu-edit-copy", "&Output"), ACT_COPY_OUTPUT, ICON_COPY_OUTPUT, L("CopyOutput"));
    SI(TN("menu-edit-copy", "&Command"), ACT_COPY_COMMAND, ICON_COPY_COMMAND, L("CopyCommand"));
    SI(TN("menu-edit-copy", "&Job"), ACT_COPY_JOB, ICON_COPY_JOB, L("CopyJob"));

    SUBMENU("menu-edit-select", TN("menu-edit", "Select"), ICON_SELECT);
    SI(TN("menu-edit-select", "&All"), ACT_SELECT_ALL, ICON_SELECT_ALL, L("SelectAll"));
    SI(TN("menu-edit-select", "&Screen"), ACT_SELECT_SCREEN, ICON_SELECT_SCREEN, L("SelectScreen"));
    SS();
    SI(TN("menu-edit-select", "&Output"), ACT_SELECT_OUTPUT, ICON_SELECT_OUTPUT, L("SelectOutput"));
    SI(TN("menu-edit-select", "&Command"), ACT_SELECT_COMMAND, ICON_SELECT_COMMAND, L("SelectCommand"));
    SI(TN("menu-edit-select", "&Job"), ACT_SELECT_JOB, ICON_SELECT_JOB, L("SelectJob"));

    SUBMENU_E("menu-edit-input", InputMenu, TN("menu-edit", "&Input Multiplexing"));
    SI(TN("menu-edit-input", "Set &Leader"), ACT_INPUT_SET_LEADER, ICON_INPUT_SET_LEADER, L("InputSetLeader"), DynInputLeader);
    SI(TN("menu-edit-input", "Set &Follower"), ACT_INPUT_TOGGLE_FOLLOWER, ICON_INPUT_TOGGLE_FOLLOWER, L("InputToggleFollower"), DynInputFollower);
    SS();
    SI(TN("menu-edit-input", "&Stop"), ACT_INPUT_UNSET_LEADER, ICON_INPUT_UNSET_LEADER, L("InputUnsetLeader"), DynInputMultiplexing);

    MS();
    MI(TN("menu-edit", "&Annotate Selection"), ACT_ANNOTATE_SELECTION, ICON_ANNOTATE_SELECTION, L("AnnotateSelection"), DynHaveSelection);
    MI(TN("menu-edit", "Annotate &Bottom of Screen"), ACT_ANNOTATE_SCREEN, ICON_ANNOTATE_BOTTOM, L("AnnotateScreen"));
    MI(TN("menu-edit", "Annotate &Top of Screen"), ACT_ANNOTATE_SCREEN_TOP, ICON_ANNOTATE_TOP, L("AnnotateScreen|0"));
    MS();
    a_commandMode =
    MI(TN("menu-edit", "Command &Mode"), ACT_TOGGLE_COMMAND_MODE, ICON_TOGGLE_COMMAND_MODE, L("ToggleCommandMode"));

    /* View menu */
    MENU("menu-view", TL("menu-main", "View"));

    SUBMENU_E("menu-view-tools", LayoutMenu, TN("menu-view", "&Tools"));
    a_dockTerminals =
    SI(TN("menu-view-tools", "&Terminals"), ACT_TOGGLE_TERMINALS_TOOL, ICON_RAISE_TERMINALS_TOOL, L("ToggleTerminalsTool"));
    a_dockKeymap =
    SI(TN("menu-view-tools", "&Keymap"), ACT_TOGGLE_KEYMAP_TOOL, ICON_RAISE_KEYMAP_TOOL, L("ToggleKeymapTool"));
    a_dockCommands =
    SI(TN("menu-view-tools", "&Suggestions"), ACT_TOGGLE_SUGGESTIONS_TOOL, ICON_RAISE_SUGGESTIONS_TOOL, L("ToggleSuggestionsTool"));
    a_dockSearch =
    SI(TN("menu-view-tools", "S&earch"), ACT_TOGGLE_SEARCH_TOOL, ICON_RAISE_SEARCH_TOOL, L("ToggleSearchTool"));
    a_dockFiles =
    SI(TN("menu-view-tools", "&Files"), ACT_TOGGLE_FILES_TOOL, ICON_RAISE_FILES_TOOL, L("ToggleFilesTool"));
    a_dockJobs =
    SI(TN("menu-view-tools", "&History"), ACT_TOGGLE_HISTORY_TOOL, ICON_RAISE_HISTORY_TOOL, L("ToggleHistoryTool"));
    a_dockNotes =
    SI(TN("menu-view-tools", "&Annotations"), ACT_TOGGLE_ANNOTATIONS_TOOL, ICON_RAISE_ANNOTATIONS_TOOL, L("ToggleAnnotationsTool"));
    a_dockTasks =
    SI(TN("menu-view-tools", "Tas&ks"), ACT_TOGGLE_TASKS_TOOL, ICON_RAISE_TASKS_TOOL, L("ToggleTasksTool"));
    SS();
    SI(TN("menu-view-tools", "&Marks"), ACT_TOGGLE_TERMINAL_LAYOUT, ICON_TOGGLE_MARKS_WIDGET, L("ToggleTerminalLayout|1"), DynLayoutItem1);
    SI(TN("menu-view-tools", "Scroll&bar"), ACT_TOGGLE_TERMINAL_LAYOUT, ICON_TOGGLE_SCROLLBAR_WIDGET, L("ToggleTerminalLayout|2"), DynLayoutItem2);
    SI(TN("menu-view-tools", "M&inimap"), ACT_TOGGLE_TERMINAL_LAYOUT, ICON_TOGGLE_MINIMAP_WIDGET, L("ToggleTerminalLayout|3"), DynLayoutItem3);
    SI(TN("menu-view-tools", "Timin&g"), ACT_TOGGLE_TERMINAL_LAYOUT, ICON_TOGGLE_TIMING_WIDGET, L("ToggleTerminalLayout|4"), DynLayoutItem4);
    SI(TN("menu-view-tools", "Adjust &Layout") "...", ACT_ADJUST_TERMINAL_LAYOUT, ICON_ADJUST_LAYOUT, L("AdjustTerminalLayout"));

    SUBMENU("menu-view-split", TN("menu-view", "Split &View"));
    SI(TN("menu-view-split", "Resizable &Left/Right Split"), ACT_SPLIT_VIEW_HORIZONTAL, ICON_SPLIT_VIEW_HORIZONTAL_RESIZE, L("SplitViewHorizontal"));
    SI(TN("menu-view-split", "Resizable &Top/Bottom Split"), ACT_SPLIT_VIEW_VERTICAL, ICON_SPLIT_VIEW_VERTICAL_RESIZE, L("SplitViewVertical"));
    SS();
    SI(TN("menu-view-split", "Fixed Left/&Right Split"), ACT_SPLIT_VIEW_HORIZONTAL_FIXED, ICON_SPLIT_VIEW_HORIZONTAL_FIXED, L("SplitViewHorizontalFixed"));
    SI(TN("menu-view-split", "Fixed Top/&Bottom Split"), ACT_SPLIT_VIEW_VERTICAL_FIXED, ICON_SPLIT_VIEW_VERTICAL_FIXED, L("SplitViewVerticalFixed"));
    SI(TN("menu-view-split", "Fixed &Quad Split"), ACT_SPLIT_VIEW_QUAD_FIXED, ICON_SPLIT_VIEW_QUAD_FIXED, L("SplitViewQuadFixed"));
    SS();
    SI(TN("menu-view-split", "&Close Pane"), ACT_SPLIT_VIEW_CLOSE, ICON_SPLIT_VIEW_CLOSE, L("SplitViewClose"));
    SI(TN("menu-view-split", "Close &Other Panes"), ACT_SPLIT_VIEW_CLOSE_OTHERS, ICON_SPLIT_VIEW_CLOSE_OTHERS, L("SplitViewCloseOthers"));
    SI(TN("menu-view-split", "&Expand Pane"), ACT_SPLIT_VIEW_EXPAND, ICON_SPLIT_VIEW_EXPAND, L("SplitViewExpand"));
    SI(TN("menu-view-split", "&Shrink Pane"), ACT_SPLIT_VIEW_SHRINK, ICON_SPLIT_VIEW_SHRINK, L("SplitViewShrink"));
    SI(TN("menu-view-split", "E&qualize Pane"), ACT_SPLIT_VIEW_EQUALIZE, ICON_SPLIT_VIEW_EQUALIZE, L("SplitViewEqualize"));
    SI(TN("menu-view-split", "Equalize &All Panes"), ACT_SPLIT_VIEW_EQUALIZE_ALL, ICON_SPLIT_VIEW_EQUALIZE_ALL, L("SplitViewEqualizeAll"));

    SUBMENU("menu-view-navigation", TN("menu-view", "&Navigation"));
    SI(TN("menu-view-navigation", "&Next Pane"), ACT_NEXT_PANE, ICON_NEXT_PANE, L("NextPane"));
    SI(TN("menu-view-navigation", "&Previous Pane"), ACT_PREVIOUS_PANE, ICON_PREVIOUS_PANE, L("PreviousPane"));
    SI(TN("menu-view-navigation", "Go to Pane &1"), ACT_SWITCH_PANE, ICON_SWITCH_PANE, L("SwitchPane|0"));
    SI(TN("menu-view-navigation", "Go to Pane &2"), ACT_SWITCH_PANE, ICON_SWITCH_PANE, L("SwitchPane|1"));
    SI(TN("menu-view-navigation", "Go to Pane &3"), ACT_SWITCH_PANE, ICON_SWITCH_PANE, L("SwitchPane|2"));
    SI(TN("menu-view-navigation", "Go to Pane &4"), ACT_SWITCH_PANE, ICON_SWITCH_PANE, L("SwitchPane|3"));
    SS();
    SI(TN("menu-view-navigation", "Next &Terminal"), ACT_NEXT_TERMINAL, ICON_NEXT_TERMINAL, L("NextTerminal"));
    SI(TN("menu-view-navigation", "Previous T&erminal"), ACT_PREVIOUS_TERMINAL, ICON_PREVIOUS_TERMINAL, L("PreviousTerminal"));
    SI(TN("menu-view-navigation", "Next &Server"), ACT_NEXT_SERVER, ICON_NEXT_SERVER, L("NextServer"));
    SI(TN("menu-view-navigation", "Previous Se&rver"), ACT_PREVIOUS_SERVER, ICON_PREVIOUS_SERVER, L("PreviousServer"));

    SUBMENU("menu-view-scroll", TN("menu-view", "&Scroll"));
    SI(TN("menu-view-scroll", "&First Prompt"), ACT_SCROLL_PROMPT_FIRST, ICON_SCROLL_PROMPT_FIRST, L("ScrollPromptFirst"));
    SI(TN("menu-view-scroll", "Previous &Prompt"), ACT_SCROLL_PROMPT_UP, ICON_SCROLL_PROMPT_UP, L("ScrollPromptUp"));
    SI(TN("menu-view-scroll", "Next Prompt"), ACT_SCROLL_PROMPT_DOWN, ICON_SCROLL_PROMPT_DOWN, L("ScrollPromptDown"));
    SI(TN("menu-view-scroll", "&Last Prompt"), ACT_SCROLL_PROMPT_LAST, ICON_SCROLL_PROMPT_LAST, L("ScrollPromptLast"));
    SS();
    SI(TN("menu-view-scroll", "Previous &Annotation"), ACT_SCROLL_NOTE_UP, ICON_SCROLL_ANNOTATION_UP, L("ScrollNoteUp"));
    SI(TN("menu-view-scroll", "Next Annotation"), ACT_SCROLL_NOTE_DOWN, ICON_SCROLL_ANNOTATION_DOWN, L("ScrollNoteDown"));
    SS();
    SI(TN("menu-view-scroll", "&Top"), ACT_SCROLL_TO_TOP, ICON_SCROLL_TO_TOP, L("ScrollToTop"));
    SI(TN("menu-view-scroll", "Page &Up"), ACT_SCROLL_PAGE_UP, ICON_SCROLL_PAGE_UP, L("ScrollPageUp"));
    SI(TN("menu-view-scroll", "Up one &Line"), ACT_SCROLL_LINE_UP, ICON_SCROLL_LINE_UP, L("ScrollLineUp"));
    SI(TN("menu-view-scroll", "Down &one Line"), ACT_SCROLL_LINE_DOWN, ICON_SCROLL_LINE_DOWN, L("ScrollLineDown"));
    SI(TN("menu-view-scroll", "Page &Down"), ACT_SCROLL_PAGE_DOWN, ICON_SCROLL_PAGE_DOWN, L("ScrollPageDown"));
    SI(TN("menu-view-scroll", "&Bottom"), ACT_SCROLL_TO_BOTTOM, ICON_SCROLL_TO_BOTTOM, L("ScrollToBottom"));

    MS();
    a_fullScreen =
    MI(TN("menu-view", "Full Scr&een"), ACT_TOGGLE_FULL_SCREEN, ICON_TOGGLE_FULL_SCREEN, L("ToggleFullScreen"));
    a_presMode =
    MI(TN("menu-view", "&Presentation Mode"), ACT_TOGGLE_PRESENTATION_MODE, ICON_TOGGLE_PRES_MODE, L("TogglePresentationMode"));
    a_menuBarShown =
    MI(TN("menu-view", "&Menu Bar"), ACT_TOGGLE_MENU_BAR, ICON_TOGGLE_MENU_BAR, L("ToggleMenuBar"));
    a_statusBarShown =
    MI(TN("menu-view", "Status &Bar"), ACT_TOGGLE_STATUS_BAR, ICON_TOGGLE_STATUS_BAR, L("ToggleStatusBar"));
    MS();
    MI(TN("menu-view", "&Increase Font"), ACT_INCREASE_FONT, ICON_INCREASE_FONT, L("IncreaseFont"));
    MI(TN("menu-view", "&Decrease Font"), ACT_DECREASE_FONT, ICON_DECREASE_FONT, L("DecreaseFont"));
    MI(TN("menu-view", "Adjust &Font") "...", ACT_ADJUST_TERMINAL_FONT, ICON_CHOOSE_FONT, L("AdjustTerminalFont"));
    MI(TN("menu-view", "Adjust &Colors") "...", ACT_ADJUST_TERMINAL_COLORS, ICON_CHOOSE_COLOR, L("AdjustTerminalColors"));
    MI(TN("menu-view", "&Random Color Theme"), ACT_RANDOM_TERMINAL_THEME, ICON_RANDOM_THEME, L("RandomTerminalTheme"));
    MS();
    MI(TN("menu-view", "&Undo All Adjustments"), ACT_UNDO_ALL_ADJUSTMENTS, ICON_UNDO_ALL_ADJUSTMENTS, L("UndoAllAdjustments"));

    /* Terminal menu */
    MENU_T("menu-terminal", TermMenu, TL("menu-main", "Terminal"));
    MI(TN("menu-terminal", "&Clone Terminal"), ACT_CLONE_TERMINAL, ICON_CLONE_TERMINAL, L("CloneTerminal"));
    MI(TN("menu-terminal", "Du&plicate Terminal"), ACT_DUPLICATE_TERMINAL, ICON_DUPLICATE_TERMINAL, L("CloneTerminal|1"));

    SUBMENU_E("dmenu-icon", TermIconMenu, TN("menu-terminal", "Set &Icon"), ICON_CHOOSE_ICON);
    SUBMENU_T(0, "dmenu-alert", AlertMenu, TN("menu-terminal", "Set &Alert"), ICON_SET_ALERT);

    SUBMENU("menu-terminal-order", TN("menu-terminal", "Re&order"), ICON_REORDER);
    SI(TN("menu-terminal-order", "Move to &Front"), ACT_REORDER_TERMINAL_FIRST, ICON_REORDER_TERMINAL_FIRST, L("ReorderTerminalFirst"));
    SI(TN("menu-terminal-order", "Move to &Back"), ACT_REORDER_TERMINAL_LAST, ICON_REORDER_TERMINAL_LAST, L("ReorderTerminalLast"));
    SI(TN("menu-terminal-order", "Move &Up"), ACT_REORDER_TERMINAL_FORWARD, ICON_REORDER_TERMINAL_FORWARD, L("ReorderTerminalForward"));
    SI(TN("menu-terminal-order", "Move &Down"), ACT_REORDER_TERMINAL_BACKWARD, ICON_REORDER_TERMINAL_BACKWARD, L("ReorderTerminalBackward"));

    MI(TN("menu-terminal", "&Hide Terminal"), ACT_HIDE_TERMINAL, ICON_HIDE_TERMINAL, L("HideTerminal"));
    MI(TN("menu-terminal", "&Show All Terminals"), ACT_SHOW_TERMINALS, ICON_SHOW_TERMINALS, L("ShowTerminal"));
    MS();
    MI(TN("menu-terminal", "&Take Ownership"), ACT_TAKE_TERMINAL_OWNERSHIP, ICON_TAKE_TERMINAL_OWNERSHIP, L("TakeTerminalOwnership"), DynNotOurTerm);
    MI(TN("menu-terminal", "Allo&w Remote Input"), ACT_TOGGLE_TERMINAL_REMOTE_INPUT, ICON_TOGGLE_TERMINAL_REMOTE_INPUT, L("ToggleTerminalRemoteInput"), DynInputtable);
    MI(TN("menu-terminal", "Scroll with &Owner"), ACT_TOGGLE_TERMINAL_FOLLOWING, ICON_NONE, L("ToggleTerminalFollowing"), DynFollowable);
    MS();
    MI(TN("menu-terminal", "&Reset Emulator"), ACT_RESET_TERMINAL, ICON_RESET_TERMINAL, L("ResetTerminal"));

    SUBMENU("menu-terminal-scroll", TN("menu-terminal", "Scroll&back"), ICON_ADJUST_SCROLLBACK);
    SI(TN("menu-terminal-scroll", "Clear Scrollback and &Reset"), ACT_RESET_AND_CLEAR_TERMINAL, ICON_RESET_AND_CLEAR_TERMINAL, L("ResetAndClearTerminal"));
    SI(TN("menu-terminal-scroll", "&Clear Scrollback"), ACT_CLEAR_TERMINAL_SCROLLBACK, ICON_CLEAR_SCROLLBACK, L("ClearTerminalScrollback"));
    SI(TN("menu-terminal-scroll", "&Adjust Scrollback") "...", ACT_ADJUST_TERMINAL_SCROLLBACK, ICON_ADJUST_SCROLLBACK, L("AdjustTerminalScrollback"));

    SUBMENU("menu-terminal-signal", TN("menu-terminal", "Send Si&gnal"), ICON_SEND_SIGNAL);
    SI(TN("menu-terminal-signal", "&Hangup (HUP)"), ACT_SEND_SIGNAL, ICON_NONE, L("SendSignal|1"));
    SI(TN("menu-terminal-signal", "&Interrupt (INT)"), ACT_SEND_SIGNAL, ICON_NONE, L("SendSignal|2"));
    SI(TN("menu-terminal-signal", "&Kill (KILL)"), ACT_SEND_SIGNAL, ICON_NONE, L("SendSignal|9"));
    SI(TN("menu-terminal-signal", "User &1 (USR1)"), ACT_SEND_SIGNAL, ICON_NONE, L("SendSignal|10"));
    SI(TN("menu-terminal-signal", "User &2 (USR2)"), ACT_SEND_SIGNAL, ICON_NONE, L("SendSignal|12"));
    SI(TN("menu-terminal-signal", "&Terminate (TERM)"), ACT_SEND_SIGNAL, ICON_NONE, L("SendSignal|15"));
    SI(TN("menu-terminal-signal", "&Continue (CONT)"), ACT_SEND_SIGNAL, ICON_NONE, L("SendSignal|18"));
    SI(TN("menu-terminal-signal", "&Stop (STOP)"), ACT_SEND_SIGNAL, ICON_NONE, L("SendSignal|19"));

    MI(TN("menu-terminal", "&View Terminal Information"), ACT_VIEW_TERMINAL_INFO, ICON_VIEW_TERMINAL_INFO, L("ViewTerminalInfo"));
    MS();
    MI(TN("menu-terminal", "&Disconnect Peer"), ACT_DISCONNECT_TERMINAL, ICON_DISCONNECT_TERMINAL, L("DisconnectTerminal"), DynHasPeer);
    MI(TN("menu-terminal", "C&lose Terminal"), ACT_CLOSE_TERMINAL, ICON_CLOSE_TERMINAL, L("CloseTerminal"));

    /* Server menu */
    MENU_S("menu-server", ServerMenu, TL("menu-main", "Server"));
    MI(TN("menu-server", "&New Terminal on %1"), ACT_NEW_TERMINAL, ICON_NEW_TERMINAL, L("NewTerminal"), DynNewTerminal);

    SUBMENU_S(0, "dmenu-newterm", NewTermMenu, TN("menu-server", "New &Terminal with Profile"));
    SUBMENU_S(0, "dmenu-newwin", NewWindowMenu, TN("menu-server", "New &Window with Profile"), ICON_NEW_WINDOW);

    MS();
    MI(TN("menu-server", "&Hide Terminals"), ACT_HIDE_SERVER, ICON_HIDE_SERVER, L("HideServer"));
    MI(TN("menu-server", "&Show Terminals"), ACT_SHOW_SERVER, ICON_SHOW_SERVER, L("ShowServer"));

    SUBMENU_E("dmenu-icon", ServerIconMenu, TN("menu-server", "Set &Icon"), ICON_CHOOSE_ICON);

    SUBMENU("menu-server-order", TN("menu-server", "Reorder"), ICON_REORDER);
    SI(TN("menu-server-order", "Move to &Front"), ACT_REORDER_SERVER_FIRST, ICON_REORDER_SERVER_FIRST, L("ReorderServerFirst"));
    SI(TN("menu-server-order", "Move to &Back"), ACT_REORDER_SERVER_LAST, ICON_REORDER_SERVER_LAST, L("ReorderServerLast"));
    SI(TN("menu-server-order", "Move &Up"), ACT_REORDER_SERVER_FORWARD, ICON_REORDER_SERVER_FORWARD, L("ReorderServerForward"));
    SI(TN("menu-server-order", "Move &Down"), ACT_REORDER_SERVER_BACKWARD, ICON_REORDER_SERVER_BACKWARD, L("ReorderServerBackward"));

    MS();
    MI(TN("menu-server", "&Edit Server \"%1\""), ACT_EDIT_SERVER, ICON_EDIT_SERVER, L("EditServer"), DynEditServer);
    MI(TN("menu-server", "&View Server Information"), ACT_VIEW_SERVER_INFO, ICON_VIEW_SERVER_INFO, L("ViewServerInfo"));
    MI(TN("menu-server", "&Manage Servers") "...", ACT_MANAGE_SERVERS, ICON_MANAGE_SERVERS, L("ManageServers"));
    MI(TN("menu-connect", "&Port Forwarding") "...", ACT_MANAGE_PORT_FORWARDING, ICON_MANAGE_PORT_FORWARDING, L("ManagePortForwarding"));
    MS();
    MI(TN("menu-server", "&Disconnect"), ACT_DISCONNECT_SERVER, ICON_DISCONNECT_SERVER, L("DisconnectServer"));

    /* Connect menu */
    MENU_E("menu-connect", ConnectMenu, TL("menu-main", "Connect"));

    SUBMENU_E("dmenu-newconn", NewConnMenu, TN("menu-connect", "&Favorites"), ICON_MENU_FAVORITES);

    MS();
    MI(TN("menu-connect", "&Transient local server"), ACT_CONN_TRANSIENT, ICON_CONNTYPE_TRANSIENT, L("OpenConnection|") + g_str_TRANSIENT_CONN, DynTransientConn);
    MI(TN("menu-connect", "&Persistent local server"), ACT_CONN_PERSISTENT, ICON_CONNTYPE_PERSISTENT, L("OpenConnection|") + g_str_PERSISTENT_CONN, DynPersistentConn);
    MS();
    MF(DynConnItem);
    MF(DynConnItem);
    MI(TN("menu-connect", "&SSH Connection") "...", ACT_CONN_SSH, ICON_CONNTYPE_SSH, L("NewConnection|%1").arg(Tsqt::ConnectionSsh));
    MS();
    MF(DynConnItem);
    MF(DynConnItem);
    MI(TN("menu-connect", "&Container") "...", ACT_CONN_CONTAINER, ICON_CONNTYPE_CONTAINER, L("NewConnection|%1").arg(Tsqt::ConnectionDocker));
    MS();
    MI(TN("menu-connect", "Superuse&r (sudo)"), ACT_CONN_ROOT, ICON_CONNTYPE_ROOT, L("NewConnection|%1|root").arg(Tsqt::ConnectionUserSudo));
    MF(DynConnItem);
    menu->updateConnectMenu();
    MI(TN("menu-connect", "Switch &User") "...", ACT_CONN_USER, ICON_CONNTYPE_USER, L("NewConnection|%1").arg(Tsqt::ConnectionUserSu));
    MS();
    MI(TN("menu-connect", "Custom Co&nnection") "...", ACT_CONN_GENERIC, ICON_CONNTYPE_GENERIC, L("NewConnection"));
    MI(TN("menu-connect", "&Manage Connections") "...", ACT_MANAGE_CONNECTIONS, ICON_MANAGE_CONNECTIONS, L("ManageConnections"));

    /* Tools menu */
    MENU_E("menu-tools", ToolsMenu, TL("menu-main", "Tools"));
    MI(TN("menu-tools", "Active Tool"), ACT_RAISE_ACTIVE_TOOL, ICON_NONE, L("RaiseActiveTool"), DynLastToolName);

    SUBMENU("menu-tools-show", TN("menu-tools", "&Activate"));
    SI(TN("menu-tools-show", "&Suggestions"), ACT_RAISE_SUGGESTIONS_TOOL, ICON_RAISE_SUGGESTIONS_TOOL, L("RaiseSuggestionsTool"));
    SI(TN("menu-tools-show", "S&earch"), ACT_RAISE_SEARCH_TOOL, ICON_RAISE_SEARCH_TOOL, L("RaiseSearchTool"));
    SI(TN("menu-tools-show", "&Files"), ACT_RAISE_FILES_TOOL, ICON_RAISE_FILES_TOOL, L("RaiseFilesTool"));
    SI(TN("menu-tools-show", "&History"), ACT_RAISE_HISTORY_TOOL, ICON_RAISE_HISTORY_TOOL, L("RaiseHistoryTool"));
    SI(TN("menu-tools-show", "&Annotations"), ACT_RAISE_ANNOTATIONS_TOOL, ICON_RAISE_ANNOTATIONS_TOOL, L("RaiseAnnotationsTool"));
    SI(TN("menu-tools-show", "Tas&ks"), ACT_RAISE_TASKS_TOOL, ICON_RAISE_TASKS_TOOL, L("RaiseTasksTool"));
    SS();
    SI(TN("menu-tools-show", "&Terminals"), ACT_RAISE_TERMINALS_TOOL, ICON_RAISE_TERMINALS_TOOL, L("RaiseTerminalsTool"));
    SI(TN("menu-tools-show", "&Keymap"), ACT_RAISE_KEYMAP_TOOL, ICON_RAISE_KEYMAP_TOOL, L("RaiseKeymapTool"));
    MS();

    // Commands tool actions
    MI(TN("menu-tools", "Write Command"), ACT_WRITE_COMMAND, ICON_WRITE_COMMAND, L("WriteSuggestion"), DynSuggestionsTool);
    MI(TN("menu-tools", "Write Command with Newline"), ACT_WRITE_COMMAND_NEWLINE, ICON_WRITE_COMMAND_NEWLINE, L("WriteSuggestionNewline"), DynSuggestionsTool);
    MI(TN("menu-tools", "Copy Command"), ACT_COPY_COMMAND, ICON_COPY_COMMAND, L("CopySuggestion"), DynSuggestionsTool);
    MI(TN("menu-tools", "Remove Command"), ACT_REMOVE_COMMAND, ICON_REMOVE_COMMAND, L("RemoveSuggestion"), DynSuggestionsTool);

    // Search tool actions
    MI(TN("menu-tools", "Search Up"), ACT_SEARCH_UP, ICON_SEARCH_UP, L("SearchUp"), DynSearchTool);
    MI(TN("menu-tools", "Search Down"), ACT_SEARCH_DOWN, ICON_SEARCH_DOWN, L("SearchDown"), DynSearchTool);

    // Jobs tool actions
    MI(TN("menu-tools", "Write Command"), ACT_WRITE_COMMAND, ICON_WRITE_COMMAND, L("WriteCommand|Tool"), DynHistoryToolAndTerm);
    MI(TN("menu-tools", "Write Command with Newline"), ACT_WRITE_COMMAND_NEWLINE, ICON_WRITE_COMMAND_NEWLINE, L("WriteCommandNewline|Tool"), DynHistoryToolAndTerm);
    MI(TN("menu-tools", "Copy Command"), ACT_COPY_COMMAND, ICON_COPY_COMMAND, L("CopyCommand|Tool"), DynHistoryToolAndTerm);
    MS_F(DynHistoryTool);
    MI(TN("menu-tools", "Jump to Start"), ACT_SCROLL_JOB_START, ICON_SCROLL_REGION_START, L("ScrollRegionStart|Tool"), DynHistoryToolAndTerm);
    MI(TN("menu-tools", "Jump to End"), ACT_SCROLL_JOB_END, ICON_SCROLL_REGION_END, L("ScrollRegionEnd|Tool"), DynHistoryToolAndTerm);
    MS_F(DynHistoryTool);

    SUBMENU_EF("menu-tools-copy", ToolsMenu, DynHistoryTool, TN("menu-tools", "Copy"), ICON_COPY);
    SI(TN("menu-tools-copy", "Output"), ACT_COPY_OUTPUT, ICON_COPY_OUTPUT, L("CopyOutput|Tool"), DynHistoryToolAndTerm);
    SI(TN("menu-tools-copy", "Job"), ACT_COPY_JOB, ICON_COPY_JOB, L("CopyJob|Tool"), DynHistoryToolAndTerm);

    SUBMENU_EF("menu-tools-select", ToolsMenu, DynHistoryTool, TN("menu-tools", "Select"), ICON_SELECT);
    SI(TN("menu-tools-select", "Output"), ACT_SELECT_OUTPUT, ICON_SELECT_OUTPUT, L("SelectOutput|Tool"), DynHistoryToolAndTerm);
    SI(TN("menu-tools-select", "Command"), ACT_SELECT_COMMAND, ICON_SELECT_COMMAND, L("SelectCommand|Tool"), DynHistoryToolAndTerm);
    SI(TN("menu-tools-select", "Job"), ACT_SELECT_JOB, ICON_SELECT_JOB, L("SelectJob|Tool"), DynHistoryToolAndTerm);

    SUBMENU_EF("menu-tools-note", ToolsMenu, DynHistoryTool, TN("menu-tools", "Annotate"), ICON_ANNOTATE);
    SI(TN("menu-tools-note", "Top of Job"), ACT_ANNOTATE_JOB, ICON_ANNOTATE_LINE, L("AnnotateRegion|Tool"), DynHistoryToolAndTerm);
    SI(TN("menu-tools-note", "Command"), ACT_ANNOTATE_COMMAND, ICON_ANNOTATE_COMMAND, L("AnnotateCommand|Tool"), DynHistoryToolAndTerm);
    SI(TN("menu-tools-note", "Output"), ACT_ANNOTATE_OUTPUT, ICON_ANNOTATE_OUTPUT, L("AnnotateOutput|Tool"), DynHistoryToolAndTerm);

    // Notes tool actions
    MI(TN("menu-tools", "Jump to Start"), ACT_SCROLL_NOTE_START, ICON_SCROLL_REGION_START, L("ScrollRegionStart|Tool"), DynNotesToolAndTerm);
    MI(TN("menu-tools", "Jump to End"), ACT_SCROLL_NOTE_END, ICON_SCROLL_REGION_END, L("ScrollRegionEnd|Tool"), DynNotesToolAndTerm);
    MS_F(DynNotesTool);
    MI(TN("menu-tools", "Remove Annotation"), ACT_REMOVE_NOTE, ICON_REMOVE_ANNOTATION, L("RemoveNote"), DynNotesToolAndTerm);
    MI(TN("menu-tools", "Select Annotation"), ACT_SELECT_NOTE, ICON_SELECT_NOTE, L("SelectJob|Tool"), DynNotesToolAndTerm);

    // Files tool actions
    const auto elt = g_settings->defaultLauncher();
    MI(TL("menu-tools", "Open with %1").arg(elt.first), ACT_OPEN_FILE, elt.second, L("OpenFile"), DynFilesToolOpenFile);

    SUBMENU_EF("menu-tools-open", OpenFileMenu, DynFilesToolAndFileHide, TN("menu-tools", "Open with"), ICON_OPEN_FILE);
    SI(TN("menu-tools-open", "Op&en with") "...", ACT_OPEN_FILE_CHOOSE, ICON_OPEN_FILE, L("OpenFile|") + g_str_PROMPT_PROFILE);
    SS();
    SI(TN("menu-tools-open", "Mount &Read-Only"), ACT_MOUNT_FILE_RO, ICON_MOUNT_FILE_RO, L("MountFile"), DynFilesToolMountableFile);
    SI(TN("menu-tools-open", "Mount Read-&Write"), ACT_MOUNT_FILE_RW, ICON_MOUNT_FILE_RW, L("MountFile"), DynFilesToolMountableFile);
    SS();
    SI(TN("menu-tools-open", "&Copy Contents as Text"), ACT_COPY_FILE, ICON_COPY_FILE, L("CopyFile|0"));
    SI(TN("menu-tools-open", "Copy Contents as &Image"), ACT_COPY_FILE, ICON_COPY_FILE, L("CopyFile|1"));

    MI(TN("menu-tools", "Download File"), ACT_DOWNLOAD_FILE, ICON_DOWNLOAD_FILE, L("DownloadFile"), DynFilesToolAndRemoteFile);
    MI(TN("menu-tools", "Upload to Here"), ACT_UPLOAD_FILE, ICON_UPLOAD_FILE, L("UploadFile"), DynFilesToolAndRemoteFile);
    MI(TN("menu-tools", "Upload to Parent Folder"), ACT_UPLOAD_DIR, ICON_UPLOAD_DIR, L("UploadToDirectory"), DynFilesToolAndRemoteDir);
    MS_F(DynFilesTool);
    MI(TN("menu-tools", "Rename File"), ACT_RENAME_FILE, ICON_RENAME_FILE, L("RenameFile"), DynFilesToolAndFile);
    MI(TN("menu-tools", "Delete File"), ACT_DELETE_FILE, ICON_DELETE_FILE, L("DeleteFile"), DynFilesToolAndFile);
    MS_F(DynFilesTool);
    MI(TN("menu-tools", "Write Path"), ACT_WRITE_FILE_PATH, ICON_WRITE_FILE_PATH, L("WriteFilePath"), DynFilesToolAndFile);
    MI(TN("menu-tools", "Write Parent Path"), ACT_WRITE_DIRECTORY_PATH, ICON_WRITE_DIRECTORY_PATH, L("WriteDirectoryPath"), DynFilesToolAndDir);
    MI(TN("menu-tools", "Copy Path"), ACT_COPY_FILE_PATH, ICON_COPY_FILE_PATH, L("CopyFilePath"), DynFilesToolAndFile);
    MI(TN("menu-tools", "Copy Parent Path"), ACT_COPY_DIRECTORY_PATH, ICON_COPY_DIRECTORY_PATH, L("CopyDirectoryPath"), DynFilesToolAndDir);

    // Tasks tool actions
    MI(TL("menu-tools", "Open with %1").arg(elt.first), ACT_OPEN_FILE, elt.second, L("OpenTaskFile"), DynTasksToolOpenFile);

    SUBMENU_EF("menu-tools-open", OpenTaskMenu, DynTasksToolAndFileHide, TN("menu-tools", "Open with"), ICON_OPEN_FILE);
    SI(TN("menu-tools-open", "Op&en with") "...", ACT_OPEN_FILE_CHOOSE, ICON_OPEN_FILE, L("OpenTaskFile|") + g_str_PROMPT_PROFILE);
    SS();
    SI(TN("menu-tools-open", "&Copy Contents as Text"), ACT_COPY_FILE, ICON_COPY_FILE, L("CopyTaskFile|0"));
    SI(TN("menu-tools-open", "Copy Contents as &Image"), ACT_COPY_FILE, ICON_COPY_FILE, L("CopyTaskFile|1"));

    MI(TN("menu-tools", "Open Folder"), ACT_OPEN_TASK_DIRECTORY, ICON_OPEN_DIRECTORY, L("OpenTaskDirectory"), DynTasksToolAndFileHide);
    MI(TN("menu-tools", "Open Terminal"), ACT_OPEN_TASK_DIRECTORY, ICON_OPEN_TERMINAL, L("OpenTaskTerminal"), DynTasksToolAndFileHide);
    MS_F(DynTasksToolAndFileHide);
    MI(TN("menu-tools", "Write Path"), ACT_WRITE_FILE_PATH, ICON_WRITE_FILE_PATH, L("WriteTaskFilePath"), DynTasksToolAndFileHide);
    MI(TN("menu-tools", "Write Parent Path"), ACT_WRITE_DIRECTORY_PATH, ICON_WRITE_DIRECTORY_PATH, L("WriteTaskDirectoryPath"), DynTasksToolAndFileHide);
    MI(TN("menu-tools", "Copy Path"), ACT_COPY_FILE_PATH, ICON_COPY_FILE_PATH, L("CopyTaskFilePath"), DynTasksToolAndFileHide);
    MI(TN("menu-tools", "Copy Parent Path"), ACT_COPY_DIRECTORY_PATH, ICON_COPY_DIRECTORY_PATH, L("CopyTaskDirectoryPath"), DynTasksToolAndFileHide);
    MS_F(DynTasksToolAndFileHide);
    MI(TN("menu-tools", "Show Status"), ACT_INSPECT_TASK, ICON_INSPECT_ITEM, L("InspectTask"), DynTasksToolAndTask);
    MI(TN("menu-tools", "Cancel Task"), ACT_CANCEL_TASK, ICON_CANCEL_TASK, L("CancelTask"), DynTasksToolAndCancel);
    MI(TN("menu-tools", "Re-run Task"), ACT_RESTART_TASK, ICON_RESTART_TASK, L("RestartTask"), DynTasksToolAndClone);
    MS_F(DynTasksTool);
    MI(TN("menu-tools", "Remove Completed Tasks"), ACT_REMOVE_TASKS, ICON_TASK_VIEW_REMOVE_TASKS, L("RemoveTasks"), DynTasksTool);

    // Active tool actions
    MS();
    SUBMENU_EF("menu-tools-filter", ToolsMenu, DynLastToolIsFilterable, TN("menu-tools", "Filter"), ICON_FILTER);
    SI(TN("menu-tools-filter", "Exclude this Terminal"), ACT_TOOL_FILTER_EXCLUDE_TERMINAL, ICON_TOOL_FILTER_EXCLUDE_TERMINAL, L("ToolFilterExcludeTerminal"), DynHaveLastToolTerm);
    SI(TN("menu-tools-filter", "Exclude this Server"), ACT_TOOL_FILTER_EXCLUDE_SERVER, ICON_TOOL_FILTER_EXCLUDE_SERVER, L("ToolFilterExcludeServer"), DynHaveLastToolServer);
    SS();
    SI(TN("menu-tools-filter", "Include Only this Terminal"), ACT_TOOL_FILTER_SET_TERMINAL, ICON_TOOL_FILTER_SET_TERMINAL, L("ToolFilterSetTerminal"), DynHaveLastToolTerm);
    SI(TN("menu-tools-filter", "Include Only this Server"), ACT_TOOL_FILTER_SET_SERVER, ICON_TOOL_FILTER_SET_SERVER, L("ToolFilterSetServer"), DynHaveLastToolServer);
    SI(TN("menu-tools-filter", "Include the active terminal"), ACT_TOOL_FILTER_ADD_TERMINAL, ICON_TOOL_FILTER_ADD_TERMINAL, L("ToolFilterAddTerminal"));
    SI(TN("menu-tools-filter", "Include the active server"), ACT_TOOL_FILTER_ADD_SERVER, ICON_TOOL_FILTER_ADD_SERVER, L("ToolFilterAddServer"));
    SS();
    SI(TN("menu-tools-filter", "Include nothing"), ACT_TOOL_FILTER_INCLUDE_NOTHING, ICON_TOOL_FILTER_INCLUDE_NOTHING, L("ToolFilterIncludeNothing"));
    SI(TN("menu-tools-filter", "Reset Filter"), ACT_TOOL_FILTER_RESET, ICON_TOOL_FILTER_RESET, L("ToolFilterReset"));
    SS();
    SI(TN("menu-tools-filter", "Remove Closed Terminals"), ACT_TOOL_FILTER_REMOVE_CLOSED, ICON_TOOL_FILTER_REMOVE_CLOSED, L("ToolFilterRemoveClosed"));

    MI(TN("menu-tools", "Search"), ACT_TOOL_SEARCH, ICON_TOOL_SEARCH, L("ToolSearch"), DynLastToolIsSearchable);
    MI(TN("menu-tools", "Reset Search"), ACT_TOOL_SEARCH_RESET, ICON_TOOL_SEARCH_RESET, L("ToolSearchReset"), DynLastToolIsSearchable);

    SUBMENU_EF("menu-tools-display", ToolsMenu, DynLastToolDisplay, TN("menu-tools", "Display"));
    a_toolBarShown =
    SI(TN("menu-tools", "Search Bar"), ACT_TOGGLE_TOOL_SEARCH_BAR, ICON_TOGGLE_TOOL_SEARCH_BAR, L("ToggleToolSearchBar"), DynLastToolSearchBarCheck);
    a_toolHeaderShown =
    SI(TN("menu-tools", "Table Header"), ACT_TOGGLE_TOOL_TABLE_HEADER, ICON_TOGGLE_TOOL_TABLE_HEADER, L("ToggleToolTableHeader"), DynLastToolTableHeaderCheck);

    FileWidget::addDisplayActions(subMenu, this);

    /* Settings menu */
    MENU_T("menu-settings", SettingsMenu, TL("menu-main", "Settings"));
    MI(TN("menu-settings", "&Edit Profile \"%1\""), ACT_EDIT_PROFILE, ICON_EDIT_PROFILE, L("EditProfile"), DynEditProfile);
    MI(TN("menu-settings", "Edit &Keymap \"%1\""), ACT_EDIT_KEYMAP, ICON_EDIT_KEYMAP, L("EditKeymap"), DynEditKeymap);
    MI(TN("menu-settings", "Edit &Global Settings"), ACT_EDIT_GLOBAL_SETTINGS, ICON_EDIT_GLOBAL_SETTINGS, L("EditGlobalSettings"));
    MS();

    SUBMENU_T(0, "dmenu-profile", SwitchProfileMenu, TN("menu-settings", "&Switch Profile"));
    SUBMENU_T(0, "dmenu-profile", PushProfileMenu, TN("menu-settings", "P&ush Profile"), ICON_PUSH_PROFILE);

    MI(TN("menu-settings", "P&op Profile"), ACT_POP_PROFILE, ICON_POP_PROFILE, L("PopProfile"));
    MI(TN("menu-settings", "E&xtract Profile from Terminal") "...", ACT_EXTRACT_PROFILE, ICON_EXTRACT_PROFILE, L("ExtractProfile"));
    MS();
    MI(TN("menu-settings", "Manage &Profiles") "...", ACT_MANAGE_PROFILES, ICON_MANAGE_PROFILES, L("ManageProfiles"));
    MI(TN("menu-settings", "&Manage Keymaps") "...", ACT_MANAGE_KEYMAPS, ICON_MANAGE_KEYMAPS, L("ManageKeymaps"));
    MI(TN("menu-settings", "Manage &Launchers") "...", ACT_MANAGE_LAUNCHERS, ICON_MANAGE_LAUNCHERS, L("ManageLaunchers"));
    MI(TN("menu-settings", "Manage &Alerts") "...", ACT_MANAGE_ALERTS, ICON_MANAGE_ALERTS, L("ManageAlerts"));
    MS();
    MI(TN("menu-settings", "Profile Autoswitch &Rules") "...", ACT_EDIT_SWITCH_RULES, ICON_EDIT_SWITCH_RULES, L("EditSwitchRules"));
    MI(TN("menu-settings", "&Icon Autoswitch Rules") "...", ACT_EDIT_ICON_RULES, ICON_EDIT_ICON_RULES, L("EditIconRules"));

    /* Help menu */
    MENU("menu-help", TL("menu-main", "Help"));
    MI(TN("menu-help", "Visit &Website"), ACT_HELP_HOMEPAGE, ICON_HELP_HOMEPAGE, L("OpenDesktopUrl|https://" PRODUCT_DOMAIN "/main.html"));
    MI(TN("menu-help", "View &Documentation"), ACT_HELP_CONTENTS, ICON_HELP_CONTENTS, L("OpenDesktopUrl|" DOCUMENTATION_ROOT));
    MI(TN("menu-help", "&Tip of the Day"), ACT_HELP_TOTD, ICON_HELP_TOTD, L("TipOfTheDay"));
    MI(TN("menu-help", "View &Man Page"), ACT_HELP_MANPAGE, ICON_MANPAGE_TERMINAL, L("ManpageTerminal"));
    MS();
    MI(TN("menu-help", "Highlight &Cursor"), ACT_HIGHLIGHT_CURSOR, ICON_HIGHLIGHT_CURSOR, L("HighlightCursor"));
    MI(TN("menu-help", "&Highlight Inline Content"), ACT_HIGHLIGHT_SEMANTIC_REGIONS, ICON_HIGHLIGHT_REGIONS, L("HighlightSemanticRegions"));
    MI(TN("menu-help", "&Invoke an Action"), ACT_PROMPT, ICON_PROMPT, L("Prompt"));
    MS();
    MI(TN("menu-help", "Manage &Plugins"), ACT_MANAGE_PLUGINS, ICON_MANAGE_PLUGINS, L("ManagePlugins"));
    MI(TN("menu-help", "&Event Log"), ACT_EVENT_LOG, ICON_EVENT_LOG, L("EventLog"));
    MS();
    MI(TN("menu-help", "&About Application"), ACT_HELP_ABOUT, ICON_HELP_ABOUT, L("HelpAbout"));
}
