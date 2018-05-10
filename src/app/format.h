// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "attr.h"

struct FormatDef
{
    const char *description;
    const char *variable;
};

extern const FormatDef g_termFormat[];
extern const FormatDef g_serverFormat[];

#define FMTSPEC(x) "\\(" x ")"
#define FMTNL "\\n"

#define DEFAULT_THUMBNAIL_CAPTION ""

#define DEFAULT_THUMBNAIL_TOOLTIP \
    FMTSPEC(TSQ_ATTR_SESSION_TITLE) \
    FMTNL \
    FMTSPEC(TSQ_ATTR_PROC_COMM) \
    "(" FMTSPEC(TSQ_ATTR_PROC_PID) ")"

#define DEFAULT_SERVER_CAPTION \
    FMTSPEC(TSQ_ATTR_SERVER_USER) \
    FMTNL \
    FMTSPEC(TSQ_ATTR_SERVER_HOST) \
    FMTNL \
    FMTSPEC(TSQ_ATTR_SERVER_NAME)

#define DEFAULT_SERVER_TOOLTIP \
    FMTSPEC(TSQ_ATTR_SERVER_USER) \
    FMTNL \
    FMTSPEC(TSQ_ATTR_SERVER_HOST) \
    FMTNL \
    FMTSPEC(TSQ_ATTR_SERVER_NAME) \
    FMTNL \
    FMTSPEC(TSQT_ATTR_SERVER_COUNT)

#define DEFAULT_WINDOW_TITLE \
    FMTSPEC(TSQT_ATTR_INDEX) \
    ": " \
    FMTSPEC(TSQ_ATTR_PROC_COMM) \
    " - " APP_NAME

#define DEFAULT_TERM_BADGE \
    FMTSPEC(TSQ_ATTR_SERVER_NAME)
