// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "attr.h"
#include "attrstr.h"

const std::string Tsq::attr_ID(TSQ_ATTR_ID);
const std::string Tsq::attr_PEER(TSQ_ATTR_PEER);
const std::string Tsq::attr_COMMAND(TSQ_ATTR_COMMAND);
const std::string Tsq::attr_HOST(TSQ_ATTR_HOST);
const std::string Tsq::attr_ICON(TSQ_ATTR_ICON);
const std::string Tsq::attr_MACHINE_ID(TSQ_ATTR_MACHINE_ID);
const std::string Tsq::attr_NAME(TSQ_ATTR_NAME);
const std::string Tsq::attr_PRODUCT(TSQ_ATTR_PRODUCT);
const std::string Tsq::attr_FLAVOR(TSQ_ATTR_FLAVOR);
const std::string Tsq::attr_STARTED(TSQ_ATTR_STARTED);
const std::string Tsq::attr_ENCODING(TSQ_ATTR_ENCODING);
const std::string Tsq::attr_CURSOR(TSQ_ATTR_CURSOR);
const std::string Tsq::attr_PID(TSQ_ATTR_PID);
const std::string Tsq::attr_UID(TSQ_ATTR_UID);
const std::string Tsq::attr_GID(TSQ_ATTR_GID);
const std::string Tsq::attr_USER(TSQ_ATTR_USER);
const std::string Tsq::attr_USERFULL(TSQ_ATTR_USERFULL);
const std::string Tsq::attr_HOME(TSQ_ATTR_HOME);
const std::string Tsq::attr_GITDESC(TSQ_ATTR_GITDESC);
const std::string Tsq::attr_AVATAR(TSQ_ATTR_AVATAR);

const std::string Tsq::attr_PROFILE_COMMAND(TSQ_ATTR_PROFILE_COMMAND);
const std::string Tsq::attr_PROFILE_ENVIRON(TSQ_ATTR_PROFILE_ENVIRON);
const std::string Tsq::attr_PROFILE_STARTDIR(TSQ_ATTR_PROFILE_STARTDIR);
const std::string Tsq::attr_PROFILE_FLAGS(TSQ_ATTR_PROFILE_FLAGS);
const std::string Tsq::attr_PROFILE_CAPORDER(TSQ_ATTR_PROFILE_CAPORDER);
const std::string Tsq::attr_PROFILE_NFILES(TSQ_ATTR_PROFILE_NFILES);
const std::string Tsq::attr_PROFILE_EXITACTION(TSQ_ATTR_PROFILE_EXITACTION);
const std::string Tsq::attr_PROFILE_AUTOCLOSE(TSQ_ATTR_PROFILE_AUTOCLOSE);
const std::string Tsq::attr_PROFILE_AUTOCLOSETIME(TSQ_ATTR_PROFILE_AUTOCLOSETIME);
const std::string Tsq::attr_PROFILE_PROMPTNEWLINE(TSQ_ATTR_PROFILE_PROMPTNEWLINE);
const std::string Tsq::attr_PROFILE_SCROLLCLEAR(TSQ_ATTR_PROFILE_SCROLLCLEAR);
const std::string Tsq::attr_PROFILE_ENCODING(TSQ_ATTR_PROFILE_ENCODING);
const std::string Tsq::attr_PROFILE_MESSAGE(TSQ_ATTR_PROFILE_MESSAGE);

const std::string Tsq::attr_PROC_TERMIOS(TSQ_ATTR_PROC_TERMIOS);
const std::string Tsq::attr_PROC_UID(TSQ_ATTR_PROC_UID);
const std::string Tsq::attr_PROC_GID(TSQ_ATTR_PROC_GID);
const std::string Tsq::attr_PROC_PID(TSQ_ATTR_PROC_PID);
const std::string Tsq::attr_PROC_CWD(TSQ_ATTR_PROC_CWD);
const std::string Tsq::attr_PROC_COMM(TSQ_ATTR_PROC_COMM);
const std::string Tsq::attr_PROC_EXE(TSQ_ATTR_PROC_EXE);
const std::string Tsq::attr_PROC_ARGV(TSQ_ATTR_PROC_ARGV);
const std::string Tsq::attr_PROC_STATUS(TSQ_ATTR_PROC_STATUS);
const std::string Tsq::attr_PROC_OUTCOME(TSQ_ATTR_PROC_OUTCOME);
const std::string Tsq::attr_PROC_OUTCOMESTR(TSQ_ATTR_PROC_OUTCOMESTR);
const std::string Tsq::attr_PROC_EXITCODE(TSQ_ATTR_PROC_EXITCODE);
const std::string Tsq::attr_PROC_DEV(TSQ_ATTR_PROC_DEV);

const std::string Tsq::attr_SENDER_PREFIX(TSQ_ATTR_SENDER_PREFIX);
const std::string Tsq::attr_SENDER_ID(TSQ_ATTR_SENDER_ID);

const std::string Tsq::attr_OWNER_PREFIX(TSQ_ATTR_OWNER_PREFIX);
const std::string Tsq::attr_OWNER_ID(TSQ_ATTR_OWNER_ID);
const std::string Tsq::attr_OWNER_USER(TSQ_ATTR_OWNER_USER);
const std::string Tsq::attr_OWNER_HOST(TSQ_ATTR_OWNER_HOST);

const std::string Tsq::attr_PREF_PALETTE(TSQ_ATTR_PREF_PALETTE);
const std::string Tsq::attr_PREF_COMMAND(TSQ_ATTR_PREF_COMMAND);
const std::string Tsq::attr_PREF_STARTDIR(TSQ_ATTR_PREF_STARTDIR);
const std::string Tsq::attr_PREF_ENVIRON(TSQ_ATTR_PREF_ENVIRON);
const std::string Tsq::attr_PREF_LANG(TSQ_ATTR_PREF_LANG);
const std::string Tsq::attr_PREF_MESSAGE(TSQ_ATTR_PREF_MESSAGE);
const std::string Tsq::attr_PREF_INPUT(TSQ_ATTR_PREF_INPUT);

const std::string Tsq::attr_SESSION_PALETTE(TSQ_ATTR_SESSION_PALETTE);
const std::string Tsq::attr_SESSION_BADGE(TSQ_ATTR_SESSION_BADGE);

const std::string Tsq::attr_SESSION_PATH(TSQ_ATTR_SESSION_PATH);
const std::string Tsq::attr_SESSION_USER(TSQ_ATTR_SESSION_USER);
const std::string Tsq::attr_SESSION_HOST(TSQ_ATTR_SESSION_HOST);
const std::string Tsq::attr_SESSION_COLS(TSQ_ATTR_SESSION_COLS);
const std::string Tsq::attr_SESSION_ROWS(TSQ_ATTR_SESSION_ROWS);
const std::string Tsq::attr_SESSION_TITLE(TSQ_ATTR_SESSION_TITLE);
const std::string Tsq::attr_SESSION_TITLE2(TSQ_ATTR_SESSION_TITLE2);
const std::string Tsq::attr_SESSION_SIVERSION(TSQ_ATTR_SESSION_SIVERSION);
const std::string Tsq::attr_SESSION_SIPREFIX(TSQ_ATTR_SESSION_SIPREFIX);
const std::string Tsq::attr_SESSION_OSC6(TSQ_ATTR_SESSION_OSC6);
const std::string Tsq::attr_SESSION_OSC7(TSQ_ATTR_SESSION_OSC7);

const std::string Tsq::attr_CLIPBOARD_PREFIX(TSQ_ATTR_CLIPBOARD_PREFIX);

const std::string Tsq::attr_USER_PREFIX(TSQ_ATTR_USER_PREFIX);
const std::string Tsq::attr_PROP_PREFIX(TSQ_ATTR_PROP_PREFIX);

const std::string Tsq::attr_REGION_COMMAND(TSQ_ATTR_REGION_COMMAND);
const std::string Tsq::attr_REGION_EXITCODE(TSQ_ATTR_REGION_EXITCODE);
const std::string Tsq::attr_REGION_STARTED(TSQ_ATTR_REGION_STARTED);
const std::string Tsq::attr_REGION_PATH(TSQ_ATTR_REGION_PATH);
const std::string Tsq::attr_REGION_USER(TSQ_ATTR_REGION_USER);
const std::string Tsq::attr_REGION_HOST(TSQ_ATTR_REGION_HOST);
const std::string Tsq::attr_REGION_ENDED(TSQ_ATTR_REGION_ENDED);
const std::string Tsq::attr_CONTENT_ID(TSQ_ATTR_CONTENT_ID);
const std::string Tsq::attr_CONTENT_TYPE(TSQ_ATTR_CONTENT_TYPE);
const std::string Tsq::attr_CONTENT_NAME(TSQ_ATTR_CONTENT_NAME);
const std::string Tsq::attr_CONTENT_URI(TSQ_ATTR_CONTENT_URI);
const std::string Tsq::attr_CONTENT_SIZE(TSQ_ATTR_CONTENT_SIZE);
const std::string Tsq::attr_CONTENT_WIDTH(TSQ_ATTR_CONTENT_WIDTH);
const std::string Tsq::attr_CONTENT_HEIGHT(TSQ_ATTR_CONTENT_HEIGHT);
const std::string Tsq::attr_CONTENT_INLINE(TSQ_ATTR_CONTENT_INLINE);

const std::string Tsq::attr_FILE_ERROR(TSQ_ATTR_FILE_ERROR);
const std::string Tsq::attr_FILE_OVERLIMIT(TSQ_ATTR_FILE_OVERLIMIT);
const std::string Tsq::attr_FILE_LINK(TSQ_ATTR_FILE_LINK);
const std::string Tsq::attr_FILE_LINKMODE(TSQ_ATTR_FILE_LINKMODE);
const std::string Tsq::attr_FILE_ORPHAN(TSQ_ATTR_FILE_ORPHAN);
const std::string Tsq::attr_DIR_GIT_AHEAD(TSQ_ATTR_DIR_GIT_AHEAD);
const std::string Tsq::attr_DIR_GIT_BEHIND(TSQ_ATTR_DIR_GIT_BEHIND);

const std::string Tsq::attr_COMMAND_COMMAND(TSQ_ATTR_COMMAND_COMMAND);
const std::string Tsq::attr_COMMAND_ENVIRON(TSQ_ATTR_COMMAND_ENVIRON);
const std::string Tsq::attr_COMMAND_STARTDIR(TSQ_ATTR_COMMAND_STARTDIR);
const std::string Tsq::attr_COMMAND_PROTOCOL(TSQ_ATTR_COMMAND_PROTOCOL);
const std::string Tsq::attr_COMMAND_PTY(TSQ_ATTR_COMMAND_PTY);
const std::string Tsq::attr_COMMAND_KEEPALIVE(TSQ_ATTR_COMMAND_KEEPALIVE);
