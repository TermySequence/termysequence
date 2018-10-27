// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "attrbase.h"
#include "lib/attr.h"

extern void initGlobalStrings();

#define TSQT_ATTR_PREFIX        APP_NAME "."
#define TSQT_ATTR_INDEX         TSQT_ATTR_PREFIX "index"
#define TSQT_ATTR_COUNT         TSQT_ATTR_PREFIX "count"
#define TSQT_ATTR_LEADER        TSQT_ATTR_PREFIX "leader"
#define TSQT_ATTR_FOLLOWER      TSQT_ATTR_PREFIX "follower"
#define TSQT_ATTR_SERVER_COUNT  TSQ_ATTR_SERVER_PREFIX TSQT_ATTR_COUNT

#define TSQT_ATTR_REGION_MARK      TSQT_ATTR_PREFIX "_m"
#define TSQT_ATTR_REGION_ROW       TSQT_ATTR_PREFIX "_r"
#define TSQT_ATTR_REGION_STARTED   TSQT_ATTR_PREFIX "_s"
#define TSQT_ATTR_REGION_DURATION  TSQT_ATTR_PREFIX "_d"

#define TSQT_SEM_ICON           TSQT_ATTR_PREFIX TSQ_ATTR_CONTENT_ICON
#define TSQT_SEM_TOOLTIP        TSQT_ATTR_PREFIX TSQ_ATTR_CONTENT_TOOLTIP
#define TSQT_SEM_ACTION1        TSQT_ATTR_PREFIX TSQ_ATTR_CONTENT_ACTION1
#define TSQT_SEM_MENU           TSQT_ATTR_PREFIX TSQ_ATTR_CONTENT_MENU
#define TSQT_SEM_DRAG           TSQT_ATTR_PREFIX TSQ_ATTR_CONTENT_DRAG

extern const QString g_str_unknown;

// Translated strings
/* Default profile (and launcher) name */
extern QString g_str_DEFAULT_PROFILE;
/* Default keymap name */
extern QString g_str_DEFAULT_KEYMAP;
/* Transient connection name */
extern QString g_str_TRANSIENT_CONN;
/* Persistent connection name */
extern QString g_str_PERSISTENT_CONN;
/* Prompt marks */
extern QString g_str_MARK_R;
extern QString g_str_MARK_E;
extern QString g_str_MARK_S;
extern QString g_str_MARK_P;
extern QString g_str_MARK_M;
extern QString g_str_MARK_N;
/* Git file marks */
extern QString g_str_GIT_I;
extern QString g_str_GIT_U;
extern QString g_str_GIT_A;
extern QString g_str_GIT_D;
extern QString g_str_GIT_R;
extern QString g_str_GIT_S;
extern QString g_str_GIT_T;
extern QString g_str_GIT_M;

/* Special profile name signifying whatever is the global default */
extern const QString g_str_CURRENT_PROFILE;
/* Special profile name signifying whatever is the server default */
extern const QString g_str_SERVER_PROFILE;
/* Special profile name signifying prompt the user */
extern const QString g_str_PROMPT_PROFILE;
/* Special launcher name signifying use the local desktop environment */
extern const QString g_str_DESKTOP_LAUNCH;

extern const QString g_attr_TSQT_INDEX;
extern const QString g_attr_TSQT_COUNT;
extern const QString g_attr_TSQT_REGION_MARK;
extern const QString g_attr_TSQT_REGION_ROW;
extern const QString g_attr_TSQT_REGION_STARTED;
extern const QString g_attr_TSQT_REGION_DURATION;

extern const QString g_attr_SEM_ICON;
extern const QString g_attr_SEM_TOOLTIP;
extern const QString g_attr_SEM_ACTION1;
extern const QString g_attr_SEM_MENU;
extern const QString g_attr_SEM_DRAG;

extern const QString g_attr_PEER;
extern const QString g_attr_COMMAND;
extern const QString g_attr_HOST;
extern const QString g_attr_ICON;
extern const QString g_attr_NAME;
extern const QString g_attr_FLAVOR;
extern const QString g_attr_STARTED;
extern const QString g_attr_PID;
extern const QString g_attr_USER;
extern const QString g_attr_HOME;

extern const QString g_attr_SERVER_PREFIX;

extern const QString g_attr_PROFILE;
extern const QString g_attr_PROFILE_PREFIX;

extern const QString g_attr_ENCODING;

extern const QString g_attr_PROC_PREFIX;
extern const QString g_attr_PROC_TERMIOS;
extern const QString g_attr_PROC_PID;
extern const QString g_attr_PROC_CWD;
extern const QString g_attr_PROC_COMM;
extern const QString g_attr_PROC_ARGV;
extern const QString g_attr_PROC_STATUS;
extern const QString g_attr_PROC_OUTCOME;
extern const QString g_attr_PROC_OUTCOMESTR;
extern const QString g_attr_PROC_EXITCODE;

extern const QString g_attr_SENDER_ID;

extern const QString g_attr_OWNER_PREFIX;
extern const QString g_attr_OWNER_ID;
extern const QString g_attr_OWNER_USER;
extern const QString g_attr_OWNER_HOST;

extern const QString g_attr_PREF_PREFIX;
extern const QString g_attr_PREF_PALETTE;
extern const QString g_attr_PREF_DIRCOLORS;
extern const QString g_attr_PREF_FONT;
extern const QString g_attr_PREF_LAYOUT;
extern const QString g_attr_PREF_FILLS;
extern const QString g_attr_PREF_BADGE;

extern const QString g_attr_PREF_COMMAND;
extern const QString g_attr_PREF_STARTDIR;
extern const QString g_attr_PREF_ENVIRON;
extern const QString g_attr_PREF_LANG;
extern const QString g_attr_PREF_MESSAGE;
extern const QString g_attr_PREF_ROW;
extern const QString g_attr_PREF_MODTIME;
extern const QString g_attr_PREF_JOB;
extern const QString g_attr_PREF_INPUT;

extern const QString g_attr_SESSION_PREFIX;
extern const QString g_attr_SESSION_PALETTE;
extern const QString g_attr_SESSION_DIRCOLORS;
extern const QString g_attr_SESSION_FONT;
extern const QString g_attr_SESSION_LAYOUT;
extern const QString g_attr_SESSION_FILLS;
extern const QString g_attr_SESSION_BADGE;
extern const QString g_attr_SESSION_ICON;

extern const QString g_attr_SESSION_TITLE;
extern const QString g_attr_SESSION_TITLE2;
extern const QString g_attr_SESSION_SIVERSION;
extern const QString g_attr_SESSION_SISHELL;

extern const QString g_attr_CLIPBOARD_PREFIX;

extern const QString g_attr_REGION_COMMAND;
extern const QString g_attr_REGION_EXITCODE;
extern const QString g_attr_REGION_STARTED;
extern const QString g_attr_REGION_PATH;
extern const QString g_attr_REGION_USER;
extern const QString g_attr_REGION_HOST;
extern const QString g_attr_REGION_ENDED;
extern const QString g_attr_REGION_NOTECHAR;
extern const QString g_attr_REGION_NOTETEXT;
extern const QString g_attr_CONTENT_ID;
extern const QString g_attr_CONTENT_TYPE;
extern const QString g_attr_CONTENT_NAME;
extern const QString g_attr_CONTENT_URI;
extern const QString g_attr_CONTENT_SIZE;
extern const QString g_attr_CONTENT_ASPECT;
extern const QString g_attr_CONTENT_INLINE;
extern const QString g_attr_CONTENT_ICON;
extern const QString g_attr_CONTENT_TOOLTIP;
extern const QString g_attr_CONTENT_ACTION1;
extern const QString g_attr_CONTENT_MENU;
extern const QString g_attr_CONTENT_DRAG;

extern const QString g_attr_DIR_GIT;
extern const QString g_attr_DIR_GIT_DETACHED;
extern const QString g_attr_DIR_GIT_TRACK;
extern const QString g_attr_DIR_GIT_AHEAD;
extern const QString g_attr_DIR_GIT_BEHIND;

extern const QString g_attr_ENV_DIRTY;
