// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "attr.h"
#include "config.h"

const QMargins g_mtmargins;
const QString g_mtstr;

// Translated strings
QString g_str_DEFAULT_PROFILE;
QString g_str_DEFAULT_KEYMAP;
QString g_str_TRANSIENT_CONN;
QString g_str_PERSISTENT_CONN;
QString g_str_MARK_R;
QString g_str_MARK_E;
QString g_str_MARK_S;
QString g_str_MARK_P;
QString g_str_MARK_M;
QString g_str_MARK_N;
QString g_str_GIT_I;
QString g_str_GIT_U;
QString g_str_GIT_A;
QString g_str_GIT_D;
QString g_str_GIT_R;
QString g_str_GIT_S;
QString g_str_GIT_T;
QString g_str_GIT_M;

void
initGlobalStrings()
{
    g_str_DEFAULT_PROFILE = TL("settings-name", "Default");
    g_str_DEFAULT_KEYMAP = TL("settings-name", "Default");
    g_str_TRANSIENT_CONN = TL("settings-name", "Local Session Server");
    g_str_PERSISTENT_CONN = TL("settings-name", "Local User Server");

    g_str_MARK_R = TL("prompt-mark", "R", "running");
    g_str_MARK_E = TL("prompt-mark", "E", "exited");
    g_str_MARK_S = TL("prompt-mark", "S", "killed");
    g_str_MARK_P = TL("prompt-mark", "P", "prompt");
    g_str_MARK_M = TL("prompt-mark", "M", "search");
    g_str_MARK_N = TL("prompt-mark", "N", "annotation");

    g_str_GIT_I = TL("git-mark", "I", "ignored");
    g_str_GIT_U = TL("git-mark", "U", "conflict");
    g_str_GIT_A = TL("git-mark", "A", "added");
    g_str_GIT_D = TL("git-mark", "D", "deleted");
    g_str_GIT_R = TL("git-mark", "R", "renamed");
    g_str_GIT_S = TL("git-mark", "S", "staged");
    g_str_GIT_T = TL("git-mark", "T", "retyped");
    g_str_GIT_M = TL("git-mark", "M", "modified");
}

const QString g_str_unknown(L("unknown"));

const QString g_str_CURRENT_PROFILE(L("<Default>"));
const QString g_str_SERVER_PROFILE(L("<ServerDefault>"));
const QString g_str_PROMPT_PROFILE(L("<Prompt>"));
const QString g_str_DESKTOP_LAUNCH(L("<Desktop>"));

const QString g_attr_TSQT_INDEX(L(TSQT_ATTR_INDEX));
const QString g_attr_TSQT_COUNT(L(TSQT_ATTR_COUNT));
const QString g_attr_TSQT_REGION_MARK(L(TSQT_ATTR_REGION_MARK));
const QString g_attr_TSQT_REGION_ROW(L(TSQT_ATTR_REGION_ROW));
const QString g_attr_TSQT_REGION_STARTED(L(TSQT_ATTR_REGION_STARTED));
const QString g_attr_TSQT_REGION_DURATION(L(TSQT_ATTR_REGION_DURATION));

const QString g_attr_SEM_ICON(L(TSQT_SEM_ICON));
const QString g_attr_SEM_TOOLTIP(L(TSQT_SEM_TOOLTIP));
const QString g_attr_SEM_ACTION1(L(TSQT_SEM_ACTION1));
const QString g_attr_SEM_MENU(L(TSQT_SEM_MENU));
const QString g_attr_SEM_DRAG(L(TSQT_SEM_DRAG));

const QString g_attr_PEER(L(TSQ_ATTR_PEER));
const QString g_attr_COMMAND(L(TSQ_ATTR_COMMAND));
const QString g_attr_HOST(L(TSQ_ATTR_HOST));
const QString g_attr_ICON(L(TSQ_ATTR_ICON));
const QString g_attr_NAME(L(TSQ_ATTR_NAME));
const QString g_attr_FLAVOR(L(TSQ_ATTR_FLAVOR));
const QString g_attr_STARTED(L(TSQ_ATTR_STARTED));
const QString g_attr_PID(L(TSQ_ATTR_PID));
const QString g_attr_USER(L(TSQ_ATTR_USER));
const QString g_attr_HOME(L(TSQ_ATTR_HOME));

const QString g_attr_SERVER_PREFIX(L(TSQ_ATTR_SERVER_PREFIX));

const QString g_attr_PROFILE(L(TSQ_ATTR_PROFILE));
const QString g_attr_PROFILE_PREFIX(L(TSQ_ATTR_PROFILE_PREFIX));

const QString g_attr_ENCODING(L(TSQ_ATTR_ENCODING));

const QString g_attr_PROC_PREFIX(L(TSQ_ATTR_PROC_PREFIX));
const QString g_attr_PROC_TERMIOS(L(TSQ_ATTR_PROC_TERMIOS));
const QString g_attr_PROC_PID(L(TSQ_ATTR_PROC_PID));
const QString g_attr_PROC_CWD(L(TSQ_ATTR_PROC_CWD));
const QString g_attr_PROC_COMM(L(TSQ_ATTR_PROC_COMM));
const QString g_attr_PROC_EXE(L(TSQ_ATTR_PROC_EXE));
const QString g_attr_PROC_ARGV(L(TSQ_ATTR_PROC_ARGV));
const QString g_attr_PROC_STATUS(L(TSQ_ATTR_PROC_STATUS));
const QString g_attr_PROC_OUTCOME(L(TSQ_ATTR_PROC_OUTCOME));
const QString g_attr_PROC_OUTCOMESTR(L(TSQ_ATTR_PROC_OUTCOMESTR));
const QString g_attr_PROC_EXITCODE(L(TSQ_ATTR_PROC_EXITCODE));

const QString g_attr_SENDER_ID(L(TSQ_ATTR_SENDER_ID));

const QString g_attr_OWNER_PREFIX(L(TSQ_ATTR_OWNER_PREFIX));
const QString g_attr_OWNER_ID(L(TSQ_ATTR_OWNER_ID));
const QString g_attr_OWNER_USER(L(TSQ_ATTR_OWNER_USER));
const QString g_attr_OWNER_HOST(L(TSQ_ATTR_OWNER_HOST));

const QString g_attr_PREF_PREFIX(L(TSQ_ATTR_PREF_PREFIX));
const QString g_attr_PREF_PALETTE(L(TSQ_ATTR_PREF_PALETTE));
const QString g_attr_PREF_DIRCOLORS(L(TSQ_ATTR_PREF_DIRCOLORS));
const QString g_attr_PREF_FONT(L(TSQ_ATTR_PREF_FONT));
const QString g_attr_PREF_LAYOUT(L(TSQ_ATTR_PREF_LAYOUT));
const QString g_attr_PREF_FILLS(L(TSQ_ATTR_PREF_FILLS));
const QString g_attr_PREF_BADGE(L(TSQ_ATTR_PREF_BADGE));

const QString g_attr_PREF_COMMAND(L(TSQ_ATTR_PREF_COMMAND));
const QString g_attr_PREF_STARTDIR(L(TSQ_ATTR_PREF_STARTDIR));
const QString g_attr_PREF_ENVIRON(L(TSQ_ATTR_PREF_ENVIRON));
const QString g_attr_PREF_LANG(L(TSQ_ATTR_PREF_LANG));
const QString g_attr_PREF_MESSAGE(L(TSQ_ATTR_PREF_MESSAGE));
const QString g_attr_PREF_ROW(L(TSQ_ATTR_PREF_ROW));
const QString g_attr_PREF_MODTIME(L(TSQ_ATTR_PREF_MODTIME));
const QString g_attr_PREF_JOB(L(TSQ_ATTR_PREF_JOB));
const QString g_attr_PREF_INPUT(L(TSQ_ATTR_PREF_INPUT));

const QString g_attr_SESSION_PREFIX(L(TSQ_ATTR_SESSION_PREFIX));
const QString g_attr_SESSION_PALETTE(L(TSQ_ATTR_SESSION_PALETTE));
const QString g_attr_SESSION_DIRCOLORS(L(TSQ_ATTR_SESSION_DIRCOLORS));
const QString g_attr_SESSION_FONT(L(TSQ_ATTR_SESSION_FONT));
const QString g_attr_SESSION_LAYOUT(L(TSQ_ATTR_SESSION_LAYOUT));
const QString g_attr_SESSION_FILLS(L(TSQ_ATTR_SESSION_FILLS));
const QString g_attr_SESSION_BADGE(L(TSQ_ATTR_SESSION_BADGE));
const QString g_attr_SESSION_ICON(L(TSQ_ATTR_SESSION_ICON));

const QString g_attr_SESSION_TITLE(L(TSQ_ATTR_SESSION_TITLE));
const QString g_attr_SESSION_TITLE2(L(TSQ_ATTR_SESSION_TITLE2));
const QString g_attr_SESSION_SIVERSION(L(TSQ_ATTR_SESSION_SIVERSION));
const QString g_attr_SESSION_SISHELL(L(TSQ_ATTR_SESSION_SISHELL));

const QString g_attr_CLIPBOARD_PREFIX(L(TSQ_ATTR_CLIPBOARD_PREFIX));

const QString g_attr_REGION_COMMAND(L(TSQ_ATTR_REGION_COMMAND));
const QString g_attr_REGION_EXITCODE(L(TSQ_ATTR_REGION_EXITCODE));
const QString g_attr_REGION_STARTED(L(TSQ_ATTR_REGION_STARTED));
const QString g_attr_REGION_PATH(L(TSQ_ATTR_REGION_PATH));
const QString g_attr_REGION_USER(L(TSQ_ATTR_REGION_USER));
const QString g_attr_REGION_HOST(L(TSQ_ATTR_REGION_HOST));
const QString g_attr_REGION_ENDED(L(TSQ_ATTR_REGION_ENDED));
const QString g_attr_REGION_NOTECHAR(L(TSQ_ATTR_REGION_NOTECHAR));
const QString g_attr_REGION_NOTETEXT(L(TSQ_ATTR_REGION_NOTETEXT));
const QString g_attr_CONTENT_ID(L(TSQ_ATTR_CONTENT_ID));
const QString g_attr_CONTENT_TYPE(L(TSQ_ATTR_CONTENT_TYPE));
const QString g_attr_CONTENT_NAME(L(TSQ_ATTR_CONTENT_NAME));
const QString g_attr_CONTENT_URI(L(TSQ_ATTR_CONTENT_URI));
const QString g_attr_CONTENT_SIZE(L(TSQ_ATTR_CONTENT_SIZE));
const QString g_attr_CONTENT_ASPECT(L(TSQ_ATTR_CONTENT_ASPECT));
const QString g_attr_CONTENT_INLINE(L(TSQ_ATTR_CONTENT_INLINE));
const QString g_attr_CONTENT_ICON(L(TSQ_ATTR_CONTENT_ICON));
const QString g_attr_CONTENT_TOOLTIP(L(TSQ_ATTR_CONTENT_TOOLTIP));
const QString g_attr_CONTENT_ACTION1(L(TSQ_ATTR_CONTENT_ACTION1));
const QString g_attr_CONTENT_MENU(L(TSQ_ATTR_CONTENT_MENU));
const QString g_attr_CONTENT_DRAG(L(TSQ_ATTR_CONTENT_DRAG));

const QString g_attr_DIR_GIT(L(TSQ_ATTR_DIR_GIT));
const QString g_attr_DIR_GIT_DETACHED(L(TSQ_ATTR_DIR_GIT_DETACHED));
const QString g_attr_DIR_GIT_TRACK(L(TSQ_ATTR_DIR_GIT_TRACK));
const QString g_attr_DIR_GIT_AHEAD(L(TSQ_ATTR_DIR_GIT_AHEAD));
const QString g_attr_DIR_GIT_BEHIND(L(TSQ_ATTR_DIR_GIT_BEHIND));

const QString g_attr_ENV_DIRTY(L(TSQ_ATTR_ENV_DIRTY));
