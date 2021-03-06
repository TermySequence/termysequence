// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#define TSQ_ATTR_ID                     "id"
#define TSQ_ATTR_PEER                   "peer"
#define TSQ_ATTR_COMMAND                "command"
#define TSQ_ATTR_HOST                   "host"
#define TSQ_ATTR_FQDN                   "fqdn"
#define TSQ_ATTR_ICON                   "icon"
#define TSQ_ATTR_MACHINE_ID             "machine-id"
#define TSQ_ATTR_NAME                   "name"
#define TSQ_ATTR_PRODUCT                "product"
#define TSQ_ATTR_FLAVOR                 "flavor"
#define TSQ_ATTR_STARTED                "started"
#define TSQ_ATTR_EMULATOR               "emulator"
#define TSQ_ATTR_ENCODING               "encoding"
#define TSQ_ATTR_CURSOR                 "cursor"
#define TSQ_ATTR_PID                    "pid"
#define TSQ_ATTR_UID                    "uid"
#define TSQ_ATTR_GID                    "gid"
#define TSQ_ATTR_USER                   "user"
#define TSQ_ATTR_USERFULL               "userfull"
#define TSQ_ATTR_HOME                   "home"
#define TSQ_ATTR_KERNEL                 "kernel"
#define TSQ_ATTR_KERNEL_RELEASE         "kernel-release"
#define TSQ_ATTR_ARCH                   "arch"
#define TSQ_ATTR_OS_PRETTY              "os-pretty"
#define TSQ_ATTR_GITDESC                "git"
#define TSQ_ATTR_NET_IP4_ADDR           "net-ip4-address"
#define TSQ_ATTR_NET_IP6_ADDR           "net-ip6-address"
#define TSQ_ATTR_TIMEZONE               "timezone"
#define TSQ_ATTR_AVATAR                 "_avatar"
#define TSQ_ATTR_LOADAVG                "loadavg"

#define TSQ_ATTR_SERVER_PREFIX          "server."
#define TSQ_ATTR_SERVER_USER            "server.user"
#define TSQ_ATTR_SERVER_NAME            "server.name"
#define TSQ_ATTR_SERVER_HOST            "server.host"

#define TSQ_SETTING_COMMAND             "Emulator/Command"
#define TSQ_SETTING_ENVIRON             "Emulator/Environment"
#define TSQ_SETTING_STARTDIR            "Emulator/Directory"
#define TSQ_SETTING_CAPORDER            "Emulator/ScrollbackSizePower"
#define TSQ_SETTING_EXITACTION          "Emulator/ActionOnProcessExit"
#define TSQ_SETTING_AUTOCLOSE           "Emulator/AutoClose"
#define TSQ_SETTING_AUTOCLOSETIME       "Emulator/AutoCloseTime"
#define TSQ_SETTING_PROMPTNEWLINE       "Emulator/InsertNewlineBeforePrompt"
#define TSQ_SETTING_SCROLLCLEAR         "Emulator/ClearScreenByScrolling"
#define TSQ_SETTING_FLAGS               "Emulator/InitialFlags"
#define TSQ_SETTING_ENCODING            "Emulator/Encoding"
#define TSQ_SETTING_LANG                "Emulator/Language"
#define TSQ_SETTING_MESSAGE             "Emulator/StartingMessage"
#define TSQ_SETTING_NFILES              "Files/DirectorySizeLimit"

#define TSQ_ATTR_PROFILE                "profile"
#define TSQ_ATTR_PROFILE_PREFIX         "profile."
#define TSQ_ATTR_PROFILE_COMMAND        "profile." TSQ_SETTING_COMMAND
#define TSQ_ATTR_PROFILE_ENVIRON        "profile." TSQ_SETTING_ENVIRON
#define TSQ_ATTR_PROFILE_STARTDIR       "profile." TSQ_SETTING_STARTDIR
#define TSQ_ATTR_PROFILE_CAPORDER       "profile." TSQ_SETTING_CAPORDER
#define TSQ_ATTR_PROFILE_EXITACTION     "profile." TSQ_SETTING_EXITACTION
#define TSQ_ATTR_PROFILE_AUTOCLOSE      "profile." TSQ_SETTING_AUTOCLOSE
#define TSQ_ATTR_PROFILE_AUTOCLOSETIME  "profile." TSQ_SETTING_AUTOCLOSETIME
#define TSQ_ATTR_PROFILE_PROMPTNEWLINE  "profile." TSQ_SETTING_PROMPTNEWLINE
#define TSQ_ATTR_PROFILE_SCROLLCLEAR    "profile." TSQ_SETTING_SCROLLCLEAR
#define TSQ_ATTR_PROFILE_FLAGS          "profile." TSQ_SETTING_FLAGS
#define TSQ_ATTR_PROFILE_ENCODING       "profile." TSQ_SETTING_ENCODING
#define TSQ_ATTR_PROFILE_MESSAGE        "profile." TSQ_SETTING_MESSAGE
#define TSQ_ATTR_PROFILE_NFILES         "profile." TSQ_SETTING_NFILES

#define TSQ_ATTR_PROC_PREFIX            "proc."
#define TSQ_ATTR_PROC_TERMIOS           "proc.termios"
#define TSQ_ATTR_PROC_UID               "proc.uid"
#define TSQ_ATTR_PROC_GID               "proc.gid"
#define TSQ_ATTR_PROC_PID               "proc.pid"
#define TSQ_ATTR_PROC_CWD               "proc.cwd"
#define TSQ_ATTR_PROC_COMM              "proc.comm"
#define TSQ_ATTR_PROC_ARGV              "proc.argv"
#define TSQ_ATTR_PROC_STATUS            "proc.status"
#define TSQ_ATTR_PROC_OUTCOME           "proc.outcome"
#define TSQ_ATTR_PROC_OUTCOMESTR        "proc.outcomestr"
#define TSQ_ATTR_PROC_EXITCODE          "proc.rc"
#define TSQ_ATTR_PROC_DEV               "proc.dev"

#define TSQ_ATTR_SCOPE_PREFIX           "scope."
#define TSQ_ATTR_SCOPE_NAME             "scope.name"

#define TSQ_ATTR_SENDER_PREFIX          "sender."
#define TSQ_ATTR_SENDER_ID              "sender.id"

#define TSQ_ATTR_OWNER_PREFIX           "owner."
#define TSQ_ATTR_OWNER_ID               "owner.id"
#define TSQ_ATTR_OWNER_USER             "owner.user"
#define TSQ_ATTR_OWNER_HOST             "owner.host"

#define TSQ_ATTR_PREF_PREFIX            "owner-pref."
#define TSQ_ATTR_PREF_PALETTE           "owner-pref.palette"
#define TSQ_ATTR_PREF_DIRCOLORS         "owner-pref.dircolors"
#define TSQ_ATTR_PREF_FONT              "owner-pref.font"
#define TSQ_ATTR_PREF_LAYOUT            "owner-pref.layout"
#define TSQ_ATTR_PREF_FILLS             "owner-pref.fills"
#define TSQ_ATTR_PREF_BADGE             "owner-pref.badge"

#define TSQ_ATTR_PREF_COMMAND           "owner-pref.command"
#define TSQ_ATTR_PREF_STARTDIR          "owner-pref.startdir"
#define TSQ_ATTR_PREF_ENVIRON           "owner-pref.environ"
#define TSQ_ATTR_PREF_LANG              "owner-pref.lang"
#define TSQ_ATTR_PREF_MESSAGE           "owner-pref.message"
#define TSQ_ATTR_PREF_ROW               "owner-pref.row"
#define TSQ_ATTR_PREF_MODTIME           "owner-pref.modtime"
#define TSQ_ATTR_PREF_JOB               "owner-pref.job"
#define TSQ_ATTR_PREF_INPUT             "owner-pref.input"

#define TSQ_ATTR_SESSION_PREFIX         "session."
#define TSQ_ATTR_SESSION_PALETTE        "session.palette"
#define TSQ_ATTR_SESSION_DIRCOLORS      "session.dircolors"
#define TSQ_ATTR_SESSION_FONT           "session.font"
#define TSQ_ATTR_SESSION_LAYOUT         "session.layout"
#define TSQ_ATTR_SESSION_FILLS          "session.fills"
#define TSQ_ATTR_SESSION_BADGE          "session.badge"
#define TSQ_ATTR_SESSION_ICON           "session.icon"

#define TSQ_ATTR_SESSION_PATH           "session.path"
#define TSQ_ATTR_SESSION_USER           "session.username"
#define TSQ_ATTR_SESSION_HOST           "session.hostname"
#define TSQ_ATTR_SESSION_COLS           "session.columns"
#define TSQ_ATTR_SESSION_ROWS           "session.rows"
#define TSQ_ATTR_SESSION_TITLE          "session.title"
#define TSQ_ATTR_SESSION_TITLE2         "session.title2"
#define TSQ_ATTR_SESSION_SIVERSION      "session.siversion"
#define TSQ_ATTR_SESSION_SIPREFIX       "session.si"
#define TSQ_ATTR_SESSION_SISHELL        "session.sishell"
#define TSQ_ATTR_SESSION_OSC6           "session.osc6"
#define TSQ_ATTR_SESSION_OSC7           "session.osc7"

#define TSQ_ATTR_CLIPBOARD_PREFIX       "clipboard."

#define TSQ_ATTR_USER_PREFIX            "user."
#define TSQ_ATTR_PROP_PREFIX            "prop."

#define TSQ_ATTR_REGION_COMMAND         "command"
#define TSQ_ATTR_REGION_EXITCODE        "rc"
#define TSQ_ATTR_REGION_STARTED         "started"
#define TSQ_ATTR_REGION_PATH            "path"
#define TSQ_ATTR_REGION_USER            "user"
#define TSQ_ATTR_REGION_HOST            "host"
#define TSQ_ATTR_REGION_ENDED           "ended"
#define TSQ_ATTR_REGION_NOTECHAR        "char"
#define TSQ_ATTR_REGION_NOTETEXT        "text"
#define TSQ_ATTR_CONTENT_ID             "id"
#define TSQ_ATTR_CONTENT_TYPE           "type"
#define TSQ_ATTR_CONTENT_NAME           "name"
#define TSQ_ATTR_CONTENT_URI            "uri"
#define TSQ_ATTR_CONTENT_SIZE           "size"
#define TSQ_ATTR_CONTENT_WIDTH          "width"
#define TSQ_ATTR_CONTENT_HEIGHT         "height"
#define TSQ_ATTR_CONTENT_ASPECT         "preserveAspectRatio"
#define TSQ_ATTR_CONTENT_INLINE         "inline"
#define TSQ_ATTR_CONTENT_ICON           "icon"
#define TSQ_ATTR_CONTENT_TOOLTIP        "tooltip"
#define TSQ_ATTR_CONTENT_ACTION1        "action1"
#define TSQ_ATTR_CONTENT_MENU           "menu"
#define TSQ_ATTR_CONTENT_DRAG           "drag"

#define TSQ_ATTR_FILE_ERROR             "error"
#define TSQ_ATTR_FILE_OVERLIMIT         "overlimit"
#define TSQ_ATTR_FILE_LINK              "link"
#define TSQ_ATTR_FILE_LINKMODE          "linkmode"
#define TSQ_ATTR_FILE_ORPHAN            "orphan"
#define TSQ_ATTR_FILE_GIT               "git"
#define TSQ_ATTR_FILE_USER              "user"
#define TSQ_ATTR_FILE_GROUP             "group"
#define TSQ_ATTR_DIR_GIT                "git"
#define TSQ_ATTR_DIR_GIT_DETACHED       "git.detached"
#define TSQ_ATTR_DIR_GIT_TRACK          "git.track"
#define TSQ_ATTR_DIR_GIT_AHEAD          "git.ahead"
#define TSQ_ATTR_DIR_GIT_BEHIND         "git.behind"

#define TSQ_ATTR_COMMAND_COMMAND        "command"
#define TSQ_ATTR_COMMAND_ENVIRON        "environ"
#define TSQ_ATTR_COMMAND_STARTDIR       "startdir"
#define TSQ_ATTR_COMMAND_PROTOCOL       "protocol"
#define TSQ_ATTR_COMMAND_PTY            "pty"
#define TSQ_ATTR_COMMAND_KEEPALIVE      "keepalive"

#define TSQ_ATTR_ENV_PREFIX             "env."
#define TSQ_ATTR_ENV_DIRTY              "env.dirty"
#define TSQ_ATTR_ENV_NAMES              "env.names"
#define TSQ_ATTR_ENV_CURRENT            "env.current"
#define TSQ_ATTR_ENV_GOAL               "env.goal"
#define TSQ_ENV_ANSWERBACK              "*answerback="
