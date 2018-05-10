// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/thumbicon.h"
using namespace std::string_literals;
#define QI(x) ThumbIcon::fromTheme(x)

#define ICON_NONE                               ""s

#define ICON_ADJUST_LAYOUT                      "adjust-layout"s                    // adwaita/actions/document-page-setup
#define ICON_ADJUST_SCROLLBACK                  "adjust-scrollback"s                // oxygen/actions/view-history
#define ICON_ANNOTATE                           "annotate"s                         // adwaita/actions/bookmark-new
#define ICON_ANNOTATE_BOTTOM                    "annotate-bottom"s
#define ICON_ANNOTATE_COMMAND                   "annotate-command"s
#define ICON_ANNOTATE_LINE                      "annotate-line"s
#define ICON_ANNOTATE_OUTPUT                    "annotate-output"s
#define ICON_ANNOTATE_SELECTION                 "annotate-selection"s
#define ICON_ANNOTATE_TOP                       "annotate-top"s
#define ICON_CANCEL_TASK                        "cancel-task"s                      // adwaita/actions/process-stop
#define ICON_CHOOSE_COLOR                       "choose-color"s                     // adwaita/categories/applications-graphics
#define ICON_CHOOSE_FONT                        "choose-font"s                      // adwaita/apps/preferences-desktop-font
#define ICON_CHOOSE_ICON                        "choose-icon"s                      // oxygen/mimetypes/image-x-generic
#define ICON_CLEAR_ALERT                        "clear-alert"s                      // adwaita/actions/edit-clear
#define ICON_CLEAR_SCROLLBACK                   "clear-scrollback"s                 // oxygen/actions/edit-clear-history
#define ICON_CLONE_TERMINAL                     "clone-terminal"s                   // oxygen/actions/window-duplicate
#define ICON_CLOSE_SEARCH                       "search-close"s                     // adwaita/actions/window-close
#define ICON_CLOSE_TERMINAL                     "close-terminal"s                   // adwaita/actions/window-close
#define ICON_CLOSE_WINDOW                       "close-window"s                     // adwaita/actions/window-close
#define ICON_CONNECTION_CLOSE                   "connection-close"s                 // adwaita/actions/call-stop
#define ICON_CONNECTION_LAUNCH                  "connection-launch"s                // adwaita/actions/call-start
#define ICON_COPY                               "copy"s                             // oxygen/actions/edit-copy
#define ICON_COPY_ALL                           "copy-all"s
#define ICON_COPY_COMMAND                       "copy-command"s
#define ICON_COPY_DIRECTORY_PATH                "copy-directory-path"s
#define ICON_COPY_FILE                          "copy-file"s
#define ICON_COPY_FILE_PATH                     "copy-file-path"s
#define ICON_COPY_IMAGE                         "copy-image"s
#define ICON_COPY_JOB                           "copy-job"s
#define ICON_COPY_NOTE                          "copy-note"s
#define ICON_COPY_OUTPUT                        "copy-output"s
#define ICON_COPY_SCREEN                        "copy-screen"s
#define ICON_COPY_URL                           "copy-url"s
#define ICON_DECREASE_FONT                      "decrease-font"s                    // oxygen/actions/format-font-size-less
#define ICON_DELETE_FILE                        "delete-file"s                      // adwaita/actions/edit-delete
#define ICON_DISCONNECT_SERVER                  "disconnect-server"s                // adwaita/actions/call-stop
#define ICON_DISCONNECT_TERMINAL                "disconnect-terminal"s              // adwaita/actions/call-stop
#define ICON_DOWNLOAD_FILE                      "download-file"s                    // adwaita/emblems/emblem-downloads
#define ICON_DOWNLOAD_IMAGE                     "download-image"s                   // oxygen/mimetypes/image-x-generic
#define ICON_DUPLICATE_TERMINAL                 "duplicate-terminal"s               // termy/duplicate-terminal
#define ICON_EDIT_GLOBAL_SETTINGS               "edit-global-settings"s             // oxygen/actions/configure
#define ICON_EDIT_ICON_RULES                    "edit-icon-rules"s                  // oxygen/apps/preferences-desktop-icons
#define ICON_EDIT_KEYMAP                        "edit-keymap"s                      // oxygen/apps/preferences-desktop-keyboard
#define ICON_EDIT_PROFILE                       "edit-profile"s                     // adwaita/categories/preferences-desktop
#define ICON_EDIT_SERVER                        "edit-server"s                      // adwaita/categories/preferences-desktop
#define ICON_EDIT_SWITCH_RULES                  "edit-switch-rules"s                // oxygen/actions/system-switch-user
#define ICON_EVENT_LOG                          "event-log"s                        // oxygen/apps/utilities-log-viewer
#define ICON_EXIT_FULL_SCREEN                   "exit-full-screen"s                 // oxygen/actions/view-restore
#define ICON_EXIT_PRES_MODE                     "exit-pres-mode"s                   // adwaita/actions/view-restore
#define ICON_EXTRACT_PROFILE                    "extract-profile"s                  // oxygen/actions/archive-extract
#define ICON_FETCH_IMAGE                        "fetch-image"s                      // adwaita/emblems/emblem-photos
#define ICON_HELP_ABOUT                         "help-about"s                       // adwaita/actions/help-about
#define ICON_HELP_CONTENTS                      "help-contents"s                    // adwaita/actions/help-contents
#define ICON_HELP_HOMEPAGE                      "help-homepage"s                    // adwaita/apps/web-browser
#define ICON_HELP_TOTD                          "help-totd"s                        // oxygen/actions/help-hint
#define ICON_HIDE_SERVER                        "hide-server"s                      // oxygen/actions/layer-visible-off
#define ICON_HIDE_TERMINAL                      "hide-terminal"s                    // oxygen/actions/layer-visible-off
#define ICON_HIGHLIGHT_CURSOR                   "highlight-cursor"s                 // oxygen/categories/applications-education-miscellaneous
#define ICON_HIGHLIGHT_REGIONS                  "highlight-regions"s                // oxygen/actions/edit-select-all
#define ICON_INCREASE_FONT                      "increase-font"s                    // oxygen/actions/format-font-size-more
#define ICON_INPUT_TOGGLE_FOLLOWER              "input-toggle-follower"s
#define ICON_INPUT_SET_LEADER                   "input-set-leader"s
#define ICON_INPUT_UNSET_LEADER                 "input-unset-leader"s               // adwaita/actions/process-stop
#define ICON_LS_FORMAT_LONG                     "ls-format-long"s                   // termy/ls-format-long
#define ICON_LS_FORMAT_SHORT                    "ls-format-short"s                  // termy/ls-format-short
#define ICON_MANAGE_ALERTS                      "manage-alerts"s                    // oxygen/apps/preferences-desktop-notification-bell
#define ICON_MANAGE_CONNECTIONS                 "manage-connections"s               // adwaita/actions/call-start
#define ICON_MANAGE_KEYMAPS                     "manage-keymaps"s                   // adwaita/devices/input-keyboard
#define ICON_MANAGE_LAUNCHERS                   "manage-launchers"s                 // oxygen/actions/fork
#define ICON_MANAGE_PLUGINS                     "manage-plugins"s                   // adwaita/mimetypes/application-x-addon
#define ICON_MANAGE_PORT_FORWARDING             "manage-port-forwarding"s           // adwaita/status/network-transmit
#define ICON_MANAGE_PROFILES                    "manage-profiles"s                  // adwaita/categories/preferences-system
#define ICON_MANAGE_SERVERS                     "manage-servers"s                   // adwaita/apps/preferences-system-sharing
#define ICON_MANAGE_TERMINALS                   "manage-terminals"s                 // adwaita/apps/utilities-terminal
#define ICON_MANPAGE_TERMINAL                   "manpage-terminal"s                 // oxygen/mimetypes/text-troff
#define ICON_MOUNT_FILE_RO                      "mount-file-ro"s                    // adwaita/emblems/emblem-readonly
#define ICON_MOUNT_FILE_RW                      "mount-file-rw"s                    // oxygen/devices/drive-harddisk
#define ICON_NEW_TERMINAL                       "new-terminal"s                     // oxygen/actions/tab-new
#define ICON_NEW_TERMINAL_LOCAL                 "new-terminal-local"s               // oxygen/actions/tab-new
#define ICON_NEW_TERMINAL_SERVER                "new-terminal-server"s              // adwaita/apps/preferences-system-sharing
#define ICON_NEW_WINDOW                         "new-window"s                       // oxygen/actions/window-new
#define ICON_NEXT_PANE                          "next-pane"s
#define ICON_NEXT_SERVER                        "next-server"s                      // adwaita/actions/go-next
#define ICON_NEXT_TERMINAL                      "next-terminal"s                    // adwaita/actions/go-next
#define ICON_OPEN_DIRECTORY                     "open-directory"s                   // adwaita/status/folder-open
#define ICON_OPEN_FILE                          "open-file"s                        // adwaita/actions/document-open
#define ICON_OPEN_TERMINAL                      "open-terminal"s                    // adwaita/apps/utilities-terminal
#define ICON_OPEN_URL                           "open-url"s                         // adwaita/apps/web-browser
#define ICON_OPEN_WITH                          "open-with"s                        // oxygen/actions/fork
#define ICON_PASTE                              "paste"s                            // oxygen/actions/edit-paste
#define ICON_PASTE_FILE                         "paste-file"s
#define ICON_POP_PROFILE                        "pop-profile"s                      // adwaita/actions/edit-undo
#define ICON_PREVIOUS_PANE                      "previous-pane"s
#define ICON_PREVIOUS_SERVER                    "previous-server"s                  // adwaita/actions/go-previous
#define ICON_PREVIOUS_TERMINAL                  "previous-terminal"s                // adwaita/actions/go-previous
#define ICON_PROMPT                             "prompt"s                           // adwaita/actions/system-run
#define ICON_PUSH_PROFILE                       "push-profile"s                     // adwaita/actions/edit-redo
#define ICON_QUIT_APPLICATION                   "quit-application"s                 // adwaita/actions/application-exit
#define ICON_RAISE_ANNOTATIONS_TOOL             "raise-annotations-tool"s           // adwaita/actions/bookmark-new
#define ICON_RAISE_FILES_TOOL                   "raise-files-tool"s                 // adwaita/apps/system-file-manager
#define ICON_RAISE_HISTORY_TOOL                 "raise-history-tool"s               // oxygen/actions/document-open-recent
#define ICON_RAISE_KEYMAP_TOOL                  "raise-keymap-tool"s                // oxygen/apps/preferences-desktop-keyboard
#define ICON_RAISE_SEARCH_TOOL                  "raise-search-tool"s                // oxygen/actions/edit-find
#define ICON_RAISE_SUGGESTIONS_TOOL             "raise-suggestions-tool"s           // oxygen/actions/milestone
#define ICON_RAISE_TASKS_TOOL                   "raise-tasks-tool"s                 // oxygen/actions/view-task
#define ICON_RAISE_TERMINALS_TOOL               "raise-terminals-tool"s             // adwaita/apps/utilities-terminal
#define ICON_RANDOM_THEME                       "random-theme"s                     // adwaita/apps/preferences-desktop-theme
#define ICON_REMOVE_ANNOTATION                  "remove-annotation"s                // oxygen/actions/edit-delete
#define ICON_REMOVE_COMMAND                     "remove-command"s                   // oxygen/actions/edit-delete
#define ICON_RENAME_FILE                        "rename-file"s                      // oxygen/actions/edit-rename
#define ICON_REORDER_SERVER_BACKWARD            "reorder-server-backward"s          // adwaita/actions/go-next
#define ICON_REORDER_SERVER_FIRST               "reorder-server-first"s             // adwaita/actions/go-first
#define ICON_REORDER_SERVER_FORWARD             "reorder-server-forward"s           // adwaita/actions/go-previous
#define ICON_REORDER_SERVER_LAST                "reorder-server-last"s              // adwaita/actions/go-last
#define ICON_REORDER_TERMINAL_BACKWARD          "reorder-terminal-backward"s        // adwaita/actions/go-next
#define ICON_REORDER_TERMINAL_FIRST             "reorder-terminal-first"s           // adwaita/actions/go-first
#define ICON_REORDER_TERMINAL_FORWARD           "reorder-terminal-forward"s         // adwaita/actions/go-previous
#define ICON_REORDER_TERMINAL_LAST              "reorder-terminal-last"s            // adwaita/actions/go-last
#define ICON_RESET_ICON                         "reset-icon"s                       // adwaita/actions/edit-clear
#define ICON_RESET_SEARCH                       "reset-search"s                     // adwaita/actions/edit-clear
#define ICON_RESET_TERMINAL                     "reset-terminal"s                   // adwaita/actions/view-refresh
#define ICON_RESET_AND_CLEAR_TERMINAL           "reset-and-clear-terminal"s         // adwaita/actions/view-refresh
#define ICON_RESTART_TASK                       "restart-task"s                     // adwaita/actions/media-playback-start
#define ICON_SAVE                               "save"s                             // adwaita/actions/document-save
#define ICON_SAVE_ALL                           "save-all"s                         // adwaita/actions/document-save-as
#define ICON_SAVE_AS                            "save-as"s                          // adwaita/actions/document-save-as
#define ICON_SAVE_SCREEN                        "save-screen"s                      // adwaita/apps/applets-screenshooter
#define ICON_SCROLL_ANNOTATION_DOWN             "scroll-annotation-down"s
#define ICON_SCROLL_ANNOTATION_UP               "scroll-annotation-up"s
#define ICON_SCROLL_IMAGE                       "scroll-image"s                     // adwaita/actions/go-next
#define ICON_SCROLL_LINE_DOWN                   "scroll-line-down"s
#define ICON_SCROLL_LINE_UP                     "scroll-line-up"s
#define ICON_SCROLL_PAGE_DOWN                   "scroll-page-down"s                 // adwaita/actions/go-down
#define ICON_SCROLL_PAGE_UP                     "scroll-page-up"s                   // adwaita/actions/go-up
#define ICON_SCROLL_PROMPT_DOWN                 "scroll-prompt-down"s
#define ICON_SCROLL_PROMPT_FIRST                "scroll-prompt-first"s              // adwaita/actions/go-top
#define ICON_SCROLL_PROMPT_LAST                 "scroll-prompt-last"s               // adwaita/actions/go-bottom
#define ICON_SCROLL_PROMPT_UP                   "scroll-prompt-up"s
#define ICON_SCROLL_REGION_END                  "scroll-region-end"s                // adwaita/actions/go-last
#define ICON_SCROLL_REGION_START                "scroll-region-start"s              // adwaita/actions/go-first
#define ICON_SCROLL_TO_BOTTOM                   "scroll-to-bottom"s                 // adwaita/actions/go-bottom
#define ICON_SCROLL_TO_TOP                      "scroll-to-top"s                    // adwaita/actions/go-top
#define ICON_SEARCH                             "search"s                           // oxygen/actions/edit-find
#define ICON_SEARCH_DOWN                        "search-down"s                      // oxygen/actions/go-down-search
#define ICON_SEARCH_UP                          "search-up"s                        // oxygen/actions/go-up-search
#define ICON_SELECT                             "select"s                           // adwaita/actions/edit-select-all
#define ICON_SELECT_ALL                         "select-all"s
#define ICON_SELECT_COMMAND                     "select-command"s
#define ICON_SELECT_JOB                         "select-job"s
#define ICON_SELECT_NOTE                        "select-note"s
#define ICON_SELECT_OUTPUT                      "select-output"s
#define ICON_SELECT_SCREEN                      "select-screen"s
#define ICON_SEND_SIGNAL                        "send-signal"s                      // oxygen/actions/edit-bomb
#define ICON_SET_ALERT                          "set-alert"s                        // oxygen/apps/preferences-desktop-notification-bell
#define ICON_SET_NO_ICON                        "set-no-icon"s
#define ICON_SHOW_MENU_BAR                      "show-menu-bar"s                    // oxygen/actions/show-menu
#define ICON_SHOW_SERVER                        "show-server"s                      // oxygen/actions/layer-visible-on
#define ICON_SHOW_TERMINAL                      "show-terminal"s                    // oxygen/actions/layer-visible-on
#define ICON_SHOW_TERMINALS                     "show-terminals"s                   // oxygen/actions/layer-visible-on
#define ICON_SORT_ASCENDING                     "sort-ascending"s                   // adwaita/actions/view-sort-ascending
#define ICON_SORT_DESCENDING                    "sort-descending"s                  // adwaita/actions/view-sort-descending
#define ICON_SPLIT_VIEW_CLOSE                   "split-view-close"s                 // oxygen/actions/view-left-close
#define ICON_SPLIT_VIEW_CLOSE_OTHERS            "split-view-close-others"s          // oxygen/actions/view-close
#define ICON_SPLIT_VIEW_EQUALIZE                "split-view-equalize"s              // oxygen/actions/view-split-left-right
#define ICON_SPLIT_VIEW_EQUALIZE_ALL            "split-view-equalize-all"s          // oxygen/actions/view-file-columns
#define ICON_SPLIT_VIEW_EXPAND                  "split-view-expand"s                // adwaita/actions/zoom-in
#define ICON_SPLIT_VIEW_HORIZONTAL_FIXED        "split-view-horizontal-fixed"s      // oxygen/actions/view-split-left-right
#define ICON_SPLIT_VIEW_HORIZONTAL_RESIZE       "split-view-horizontal-resize"s     // oxygen/actions/view-split-left-right
#define ICON_SPLIT_VIEW_QUAD_FIXED              "split-view-quad-fixed"s            // termy/split-view-quad-fixed
#define ICON_SPLIT_VIEW_SHRINK                  "split-view-shrink"s                // adwaita/actions/zoom-out
#define ICON_SPLIT_VIEW_VERTICAL_FIXED          "split-view-vertical-fixed"s        // oxygen/actions/view-split-top-bottom
#define ICON_SPLIT_VIEW_VERTICAL_RESIZE         "split-view-vertical-resize"s       // oxygen/actions/view-split-top-bottom
#define ICON_SWITCH_PANE                        "switch-pane"s                      // adwaita/actions/go-next
#define ICON_TAKE_TERMINAL_OWNERSHIP            "take-terminal-ownership"s          // adwaita/status/avatar-default
#define ICON_TASK_VIEW_REMOVE_TASKS             "task-view-remove-tasks"s           // adwaita/actions/edit-clear-all
#define ICON_TIMING_FLOAT_ORIGIN                "timing-float-origin"s
#define ICON_TIMING_SET_ORIGIN                  "timing-set-origin"s                // adwaita/actions/mark-location
#define ICON_TOGGLE_COMMAND_MODE                "toggle-command-mode"s              // oxygen/actions/quickopen
#define ICON_TOGGLE_FULL_SCREEN                 "toggle-full-screen"s               // adwaita/actions/view-fullscreen
#define ICON_TOGGLE_PRES_MODE                   "toggle-pres-mode"s                 // oxygen/devices/video-display
#define ICON_TOGGLE_MARKS_WIDGET                "toggle-marks-widget"s              // adwaita/actions/mark-location
#define ICON_TOGGLE_MINIMAP_WIDGET              "toggle-minimap-widget"s            // adwaita/actions/edit-find
#define ICON_TOGGLE_MENU_BAR                    "toggle-menu-bar"s                  // oxygen/actions/show-menu
#define ICON_TOGGLE_SCROLLBAR_WIDGET            "toggle-scrollbar-widget"s          // termy/toggle-scrollbar-widget
#define ICON_TOGGLE_STATUS_BAR                  "toggle-status-bar"s                // adwaita/status/dialog-information
#define ICON_TOGGLE_TIMING_WIDGET               "toggle-timing-widget"s             // oxygen/status/user-away
#define ICON_TOGGLE_TOOL_SEARCH_BAR             "toggle-tool-search-bar"s           // oxygen/actions/system-search
#define ICON_TOGGLE_TOOL_TABLE_HEADER           "toggle-tool-table-header"s         // oxygen/actions/show-menu
#define ICON_TOGGLE_TERMINAL_REMOTE_INPUT       "toggle-terminal-remote-input"s     // adwaita/status/security-medium
#define ICON_TOOL_FILTER_ADD_SERVER             "tool-filter-add-server"s           // oxygen/actions/list-add
#define ICON_TOOL_FILTER_ADD_TERMINAL           "tool-filter-add-terminal"s         // oxygen/actions/list-add
#define ICON_TOOL_FILTER_EXCLUDE_SERVER         "tool-filter-exclude-server"s       // oxygen/actions/list-remove
#define ICON_TOOL_FILTER_EXCLUDE_TERMINAL       "tool-filter-exclude-terminal"s     // oxygen/actions/list-remove
#define ICON_TOOL_FILTER_INCLUDE_NOTHING        "tool-filter-include-nothing"s      // oxygen/actions/dialog-cancel
#define ICON_TOOL_FILTER_RESET                  "tool-filter-reset"s                // adwaita/actions/edit-clear
#define ICON_TOOL_FILTER_SET_SERVER             "tool-filter-set-server"s           // oxygen/actions/view-filter
#define ICON_TOOL_FILTER_SET_TERMINAL           "tool-filter-set-terminal"s         // oxygen/actions/view-filter
#define ICON_TOOL_FILTER_REMOVE_CLOSED          "tool-filter-remove-closed"s        // adwaita/actions/edit-clear-all
#define ICON_TOOL_SEARCH                        "tool-search"s                      // oxygen/actions/system-search
#define ICON_TOOL_SEARCH_RESET                  "tool-search-reset"s                // adwaita/actions/edit-clear
#define ICON_UNDO_ALL_ADJUSTMENTS               "undo-all-adjustments"s             // adwaita/actions/edit-undo
#define ICON_UPLOAD_DIR                         "upload-dir"s                       // adwaita/actions/document-send
#define ICON_UPLOAD_FILE                        "upload-file"s                      // adwaita/actions/document-send
#define ICON_VIEW_SERVER_INFO                   "view-server-info"s                 // oxygen/actions/help-about
#define ICON_VIEW_TERMINAL_CONTENT              "view-terminal-content"s            // adwaita/emblems/emblem-photos
#define ICON_VIEW_TERMINAL_INFO                 "view-terminal-info"s               // oxygen/actions/help-about
#define ICON_WRITE_COMMAND                      "write-command"s                    // oxygen/actions/edit-paste
#define ICON_WRITE_COMMAND_NEWLINE              "write-command-newline"s            // oxygen/actions/key-enter
#define ICON_WRITE_DIRECTORY_PATH               "write-directory-path"s             // oxygen/actions/edit-paste
#define ICON_WRITE_FILE_PATH                    "write-file-path"s                  // oxygen/actions/edit-paste

#define ICON_CLEAN                              "clean"s                            // adwaita/actions/edit-clear-all
#define ICON_CLEAR                              "clear"s                            // adwaita/actions/edit-clear
#define ICON_CONFIGURE                          "configure"s                        // adwaita/categories/preferences-desktop
#define ICON_DESTROY                            "destroy"s                          // oxygen/actions/edit-bomb
#define ICON_EXECUTE                            "execute"s                          // adwaita/actions/system-run
#define ICON_FILTER                             "filter"s                           // oxygen/actions/view-filter
#define ICON_PAUSE                              "pause"s                            // adwaita/actions/media-playback-pause
#define ICON_RELOAD                             "reload"s                           // adwaita/actions/view-refresh
#define ICON_RELOAD_ALL                         "reload-all"s                       // termy/reload-all
#define ICON_REORDER                            "reorder"s                          // termy/reorder
#define ICON_REBOOT                             "reboot"s                           // oxygen/actions/system-reboot
#define ICON_RESET                              "reset"s                            // adwaita/actions/view-refresh
#define ICON_RESUME                             "resume"s                           // adwaita/actions/media-playback-start
#define ICON_SHUTDOWN                           "shutdown"s                         // oxygen/actions/system-shutdown
#define ICON_APPEND_ITEM                        "append-item"s
#define ICON_CHILD_ITEM                         "child-item"s
#define ICON_CHOOSE_ITEM                        "choose-item"s                      // oxygen/actions/edit-select
#define ICON_CLONE_ITEM                         "clone-item"s                       // oxygen/actions/edit-copy
#define ICON_EDIT_ITEM                          "edit-item"s                        // adwaita/apps/accessories-text-editor
#define ICON_HIDE_ITEM                          "hide-item"s                        // oxygen/actions/layer-visible-off
#define ICON_IMPORT_ITEM                        "import-item"s                      // oxygen/actions/document-import
#define ICON_INSPECT_ITEM                       "inspect-item"s                     // oxygen/actions/help-about
#define ICON_INSERT_ITEM                        "insert-item"s                      // oxygen/actions/list-add
#define ICON_NEW_ITEM                           "new-item"s                         // oxygen/actions/list-add
#define ICON_PREPEND_ITEM                       "prepend-item"s
#define ICON_REMOVE_ITEM                        "remove-item"s                      // oxygen/actions/list-remove
#define ICON_RENAME_ITEM                        "rename-item"s                      // oxygen/actions/edit-rename
#define ICON_SHOW_ITEM                          "show-item"s                        // oxygen/actions/layer-visible-on
#define ICON_SUBMIT_ITEM                        "submit-item"s                      // oxygen/actions/key-enter
#define ICON_SWITCH_ITEM                        "switch-item"s                      // adwaita/actions/go-next
#define ICON_MOVE_TOP                           "move-top"s                         // adwaita/actions/go-top
#define ICON_MOVE_UP                            "move-up"s                          // adwaita/actions/go-up
#define ICON_MOVE_DOWN                          "move-down"s                        // adwaita/actions/go-down
#define ICON_MOVE_BOTTOM                        "move-bottom"s                      // adwaita/actions/go-bottom

#define ICON_MENU_FAVORITES                     "menu-favorites"s                   // oxygen/places/favorites
#define ICON_MENU_TERMINAL                      "menu-terminal"s                    // adwaita/apps/utilities-terminal

#define ICON_CONNTYPE_PERSISTENT                "conntype-persistent"               // adwaita/apps/utilities-terminal
#define ICON_CONNTYPE_TRANSIENT                 "conntype-transient"                // oxygen/places/user-desktop
#define ICON_CONNTYPE_BATCH                     "conntype-batch"                    // adwaita/mimetypes/application-x-executable
#define ICON_CONNTYPE_GENERIC                   "conntype-generic"                  // adwaita/categories/preferences-desktop
#define ICON_CONNTYPE_SSH                       "conntype-ssh"                      // adwaita/places/network-server
#define ICON_CONNTYPE_USER                      "conntype-user"                     // adwaita/status/avatar-default
#define ICON_CONNTYPE_CONTAINER                 "conntype-container"                // oxygen/status/wallet-closed
#define ICON_CONNTYPE_ACTIVE                    QI("conntype-active")               // oxygen/actions/network-connect
#define ICON_CONNTYPE_NAMED                     QI("conntype-named")                // adwaita/mimetypes/text-x-generic
#define ICON_CONNTYPE_ROOT                      "conntype-root"s                    // oxygen/actions/irc-operator

#define ICON_LAUNCHTYPE_DEFAULT                 "launchtype-default"                // oxygen/actions/fork
#define ICON_LAUNCHTYPE_RUN_LOCAL               "launchtype-run-local"              // adwaita/actions/system-run
#define ICON_LAUNCHTYPE_RUN_REMOTE              "launchtype-run-remote"             // adwaita/actions/system-run
#define ICON_LAUNCHTYPE_TERM_LOCAL              "launchtype-term-local"             // adwaita/apps/utilities-terminal
#define ICON_LAUNCHTYPE_TERM_REMOTE             "launchtype-term-remote"            // adwaita/apps/utilities-terminal
#define ICON_LAUNCHTYPE_WRITE                   "launchtype-write"                  // oxygen/actions/edit-paste

#define ICON_TASKTYPE_DOWNLOAD_FILE             QI("tasktype-download-file")        // adwaita/emblems/emblem-downloads
#define ICON_TASKTYPE_DOWNLOAD_IMAGE            QI("tasktype-download-image")       // oxygen/mimetypes/image-x-generic
#define ICON_TASKTYPE_DOWNLOAD_PIPE             QI("tasktype-download-pipe")        // adwaita/actions/mail-send-receive
#define ICON_TASKTYPE_COPY_FILE                 QI("tasktype-copy-file")            // oxygen/actions/edit-copy
#define ICON_TASKTYPE_FETCH_IMAGE               QI("tasktype-fetch-image")          // adwaita/emblems/emblem-photos
#define ICON_TASKTYPE_DELETE_FILE               QI("tasktype-delete-file")          // adwaita/actions/edit-delete
#define ICON_TASKTYPE_RENAME_FILE               QI("tasktype-rename-file")          // oxygen/actions/edit-rename
#define ICON_TASKTYPE_LOCAL_COMMAND             QI("tasktype-local-command")        // adwaita/actions/system-run
#define ICON_TASKTYPE_REMOTE_COMMAND            QI("tasktype-remote-command")       // adwaita/actions/system-run
#define ICON_TASKTYPE_RUN_CONNECT               QI("tasktype-run-connect")          // adwaita/actions/call-start
#define ICON_TASKTYPE_RUN_BATCH                 QI("tasktype-run-batch")            // adwaita/mimetypes/application-x-executable
#define ICON_TASKTYPE_PASTE_FILE                QI("tasktype-paste-file")           // oxygen/actions/edit-paste
#define ICON_TASKTYPE_PASTE_BYTES               QI("tasktype-paste-bytes")          // oxygen/actions/edit-paste
#define ICON_TASKTYPE_PORT_FORWARD_IN           QI("tasktype-port-forward-in")      // adwaita/status/network-receive
#define ICON_TASKTYPE_PORT_FORWARD_OUT          QI("tasktype-port-forward-out")     // adwaita/status/network-transmit
#define ICON_TASKTYPE_UPLOAD_FILE               QI("tasktype-upload-file")          // adwaita/actions/document-send
#define ICON_TASKTYPE_UPLOAD_PIPE               QI("tasktype-upload-pipe")          // adwaita/actions/mail-send-receive
#define ICON_TASKTYPE_MOUNT                     QI("tasktype-mount")                // oxygen/devices/drive-harddisk

#define ICON_PLUGIN_DISABLED                    QI("plugin-disabled")               // adwaita/status/dialog-error
#define ICON_PLUGIN_PARSER                      QI("plugin-parser")                 // oxygen/actions/code-context
#define ICON_PLUGIN_ACTION                      QI("plugin-action")                 // adwaita/actions/system-run
#define ICON_PLUGIN_TOTD                        QI("plugin-totd")                   // oxygen/actions/help-hint
