// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "../config.h"

/* File containing global settings */
#define APP_CONFIG APP_NAME ".conf"
/* File containing saved state */
#define APP_STATE APP_NAME ".state"
/* File containing profile switching rules */
#define APP_SWITCHRULES "switch.rules"
/* File containing icon switching rules */
#define APP_ICONRULES "icon.rules"
/* Folder containing profiles */
#define APP_KEYMAPDIR "keymaps"
/* Folder containing connections */
#define APP_PROFILEDIR "profiles"
/* Folder containing launchers */
#define APP_LAUNCHDIR "launchers"
/* Folder containing connections */
#define APP_CONNDIR "connections"
/* Folder containing servers */
#define APP_SERVERDIR "servers"
/* Folder containing alerts */
#define APP_ALERTDIR "alerts"
/* Folder containing themes */
#define APP_THEMEDIR "themes"
/* File containing datastore */
#define APP_DATASTORE "history.sqlite3"
/* File containing avatar */
#define APP_AVATAR "avatar.svg"
/* Default icon theme directory */
#define DEFAULT_ICON_THEME "default"

/* Datastore minimum command size */
#define DATASTORE_MINIMUM_LENGTH 3
/* Datastore acronym size limit */
#define DATASTORE_ACRONYM_LIMIT 10
/* Datastore exclusion size limit */
#define DATASTORE_EXCLUSION_LIMIT 128
/* Plugin watchdog timeout interval */
#define PLUGIN_WATCHDOG_INTERVAL 1500
/* Number of watchdog timeout intervals before action taken */
#define PLUGIN_WATCHDOG_THRESHOLD 3

/* Keymap file extension */
#define KEYMAP_EXT ".keymap"
/* Profile file extension */
#define PROFILE_EXT ".profile"
/* Launcher file extension */
#define LAUNCH_EXT ".launch"
/* Connection file extension */
#define CONN_EXT ".conn"
/* Server file extension */
#define SERVER_EXT ".server"
/* Alert file extension */
#define ALERT_EXT ".alert"
/* Theme file extension */
#define THEME_EXT ".theme"

/* Default number of items shown in chooser menus */
#define ITEM_MENU_SIZE 8
/* Maximum number of items shown in chooser menus */
#define ITEM_MENU_MAX 32
/* Maximum number of items shown in peek popup (must be odd) */
#define PEEK_MAX 7
/* Maximum profile stack size */
#define PROFILE_STACK_MAX 8
/* Maximum paste size that does not launch a task */
#define PASTE_SIZE_THRESHOLD 524288

/* Time to wait for additional server registrations */
#define POPULATE_TIME 1000
/* Time to wait before showing the connection status dialog */
#define CONNECT_TIME 5000
/* Time to wait before autohiding tasks tool and task status dialogs(half) */
#define TASK_STATUS_TIME 4000
/* Time to wait before autoshowing suggestions tool after a search hit */
#define SUGGEST_DELAY_TIME 500
/* Time for terminal peek popup */
#define PEEK_TIME 1000
/* Time to wait for peer in a connect task before giving up */
#define TASK_CONNECT_TIME 30000
/* Time to wait after directory load before displaying update animation */
#define DIRLOAD_TIME 2000
/* Time period in which file updates will be coalesced */
#define DIRDEFER_TIME 100
/* Minutes before unmounting a launched mounted file that was never opened */
#define MOUNT_IDLE_TIMEOUT 10

/* Minimum badge scroll rate */
#define BADGE_RATE_MIN 100
/* Default badge scroll rate */
#define BADGE_RATE_DEFAULT 1000
/* Badge scroll pause duration */
#define BADGE_RATE_PAUSE 5000
/* Exit effect default runtime */
#define EXIT_EFFECT_RUNTIME 2000

/* Time unit for blinks (must be multiple of 100) */
#define BLINK_TIME 500
/* Minimum blink time */
#define BLINK_TIME_MIN 250
/* Number of blinks for cursor */
#define BLINK_CURSOR_COUNT 10
/* Number of blinks to skip after keypress */
#define BLINK_SKIP_COUNT 1
/* Number of blinks for text */
#define BLINK_TEXT_COUNT 20

/* Time unit for effects */
#define EFFECT_TIME 125
/* Number of time units for resize popup */
#define EFFECT_RESIZE_DURATION 20
/* Number of time units for resize popup fade */
#define EFFECT_RESIZE_FADE 4
/* Number of time units for focus effect */
#define EFFECT_FOCUS_DURATION 8
/* Number of time units for focus effect fade */
#define EFFECT_FOCUS_FADE 4

/* Time unit for major status messages */
#define MAJOR_MESSAGE_TIME 10000
/* Time unit for minor status messages */
#define MINOR_MESSAGE_TIME 8000
/* Cursor highlight time */
#define CURSOR_HIGHLIGHT_TIME 500

/* Divisor for split pane expand/shrink */
#define VIEW_EXPAND_SECTION 20

/* Default dock height */
#define DOCK_SCALE 1/5
/* Minimum thumbnail dimension as screen divisor */
#define THUMB_MIN_SCALE 16
/* Minimum thumbnail dimension as absolute */
#define THUMB_MIN_SIZE 100
/* Margin surrounding thumbnail */
#define THUMB_MARGIN 5
/* Separation between thumbnail and text */
#define THUMB_SEP 5
/* Thumbnail icon offset */
#define ICON_OFF 6
/* Thumbnail icon max scaled width */
#define ICON_WSCALE 2/3
/* Thumbnail icon max scaled height */
#define ICON_HSCALE 1/2
/* Thumbnail indicator max scaled width */
#define INDICATOR_WSCALE 2/5
/* Thumbnail indicator max scaled height */
#define INDICATOR_HSCALE 2/5
/* Index background scale */
#define INDEX_SCALE 16
/* Terminal badge scale */
#define BADGE_SCALE 4
/* Size of pixmaps used to create icons */
#define PIXMAP_ICON_SIZE 128

/* Side of drag and drop triangle */
#define CARET_WIDTH 6
/* Area on either side where drag autoscroll occurs*/
#define DRAG_AUTOSCROLL_MARGIN (THUMB_MIN_SIZE - 1)
/* Time unit for drag-selection */
#define DRAG_SELECT_TIME 100
/* Time unit for drag-scrolling */
#define DRAG_SCROLL_TIME 200
/* Mime type for drag and drop reordering */
#define REORDER_MIME_TYPE "text/x-" APP_NAME "-rearrangement"

/* Increment at which terminal area spacing/margin increases */
#define MARGIN_INCREMENT 32
/* Increment at which cursor inactive line width increases */
#define CURSOR_BOX_INCREMENT 36
/* Increment at which special-drawing line width increases */
#define U2500_WIDTH_INCREMENT 32

/* Default terminal layout */
#define TERM_LAYOUT "-4,1,s,0,-2,3"
/* Default column headers in history tool */
#define DEFAULT_JOBVIEW_COLUMNS {"0","2","3","7"}
/* Default column headers in notes tool */
#define DEFAULT_NOTEVIEW_COLUMNS {"0","2","5"}
/* Default column headers in tasks tool */
#define DEFAULT_TASKVIEW_COLUMNS {"0","1","3","5","10"}
/* Default column headers in files tool */
#define DEFAULT_FILEVIEW_COLUMNS {"0","1","2","3","4","6"}

/* Default history tool row limit */
#define DEFAULT_JOBVIEW_LIMIT 1000
/* Minimum tasks tool row limit */
#define MIN_TASKVIEW_LIMIT 10
/* Default tasks tool row limit */
#define DEFAULT_TASKVIEW_LIMIT 1000
/* Default inline content drag start multiplier */
#define DEFAULT_INLINE_DRAG 2
/* Default number of prompts to show in minimap */
#define DEFAULT_MINIMAP_PROMPTS 48
/* Maximum number of prompts to show in minimap */
#define MAX_MINIMAP_PROMPTS 255
/* Maximum number of notes tool row animations */
#define MAX_NOTE_ANIMATIONS 4
/* Maximum number of jobs tool row animations */
#define MAX_JOB_ANIMATIONS 8
/* Maximum number of files tool row animations */
#define MAX_FILE_ANIMATIONS 48

/* Number of spaces before drawing a new string */
#define CELL_SPLIT_THRESHOLD 16

/* Maximum number of lines to check when moving selection */
#define MAX_SELECTION_LINES 512
/* Maximum number of continuation lines to check for links */
#define MAX_CONTINUATION 64
/* Number of additional lines to prefetch when scanning */
#define SCAN_PREFETCH 255
/* Number of lines to process/fetch per fetchtimer iteration */
#define FETCH_PREFETCH 50
/* Minimum idle time before fetchtimer will act */
#define FETCH_IDLE_MIN 50
/* Number of terminals handled per fetchtimer callback */
#define FETCH_BATCH_SIZE 8
/* Number of lines to process per semantic parser iteration */
#define SEMANTIC_BATCH_SIZE 256

/* Maximum semantic region tooltip length */
#define SEMANTIC_TOOLTIP_MAX 100
/* Maximum semantic region menu size */
#define SEMANTIC_MENU_MAX 25

/* Validity period for FUSE objects that can change on the server */
#define FUSE_REMOTE_VALIDITY_PERIOD 10.0
/* Validity period for FUSE objects that the client controls */
#define FUSE_LOCAL_VALIDITY_PERIOD 3600.0
