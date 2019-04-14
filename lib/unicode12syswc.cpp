// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "util.h"
#include "unicode.h"
#include "utf8.h"

#define PLUGIN_NAME     TSQ_UNICODE_VARIANT_SYSWC
#define PLUGIN_REVISION TSQ_UNICODE_REVISION_SYSWC

#ifdef PLUGIN
#include "unicode12data.cpp"
#else
#include "unicode12data.h"
#endif
#include <cwchar>

//
// SystemLocale methods
//
static Tsq::CellFlags
syswc_lookup(codepoint_t c, MainTablePtr table)
{
    switch(wcwidth(c)) {
    case -1:
        return GcbInvalid;
    case 0:
        return GcbCombining;
    case 1:
        return table ? table->lookup(c) & PerCharFlags : 0;
    default:
        return DblWidthChar | (g_single_ambig_table.lookup(c) & EmojiChar);
    }
}

static Tsq::CellFlags
syswc_width(codepoint_t c, MainTablePtr table)
{
    if (wcwidth(c) != 2) {
        return table ? table->lookup(c) & DblWidthChar : 0;
    } else {
        return DblWidthChar;
    }
}

static inline bool
syswc_combines(codepoint_t c)
{
    return wcwidth(c) == 0;
}

static int32_t
widthAt(const UnicodingImpl *m, const char *i, const char *j)
{
    auto table = (MainTablePtr)m->privdata;
    codepoint_t c = utf8::unchecked::next(i);
    return (syswc_width(c, table) != 0) + 1;
}

static int32_t
widthNext(UnicodingImpl *m, const char **i, const char *j)
{
    auto table = (MainTablePtr)m->privdata;
    codepoint_t c = utf8::unchecked::next(*i);
    Tsq::CellFlags gcb = syswc_lookup(c, table);

    m->nextFlags = gcb & PerCharFlags;
    m->nextLen = 1;
    m->nextSeq[0] = c;

    while (*i != j) {
        auto k = *i;
        c = utf8::unchecked::next(k);

        if (!(syswc_combines(c)))
            break;

        switch (c) {
        case EMOJI_SELECTOR:
            if (m->nextFlags == DblWidthChar)
                m->nextFlags |= EmojiChar;
            break;
        case TEXT_SELECTOR:
            m->nextFlags &= ~EmojiChar;
            break;
        }

        *i = k;
        m->nextSeq[m->nextLen] = c;
        m->nextLen += (m->nextLen < MAX_CLUSTER_SIZE);
    }
    return (m->nextFlags != 0) + 1;
}

static int32_t
widthCategoryOf(UnicodingImpl *m, codepoint_t c, Tsq::CellFlags *flagsor)
{
    auto table = (MainTablePtr)m->privdata;
    Tsq::CellFlags gcb = syswc_lookup(c, table);

    switch (gcb & GcbBaseMask) {
    case GcbInvalid:
        m->flags = syswc_width(REPLACEMENT_CH, table);
        goto replace;
    case GcbCombining:
        if (m->len == 0)
            goto assign;
        switch (c) {
        case EMOJI_SELECTOR:
            if (m->flags == DblWidthChar)
                m->flags |= EmojiChar;
            break;
        case TEXT_SELECTOR:
            m->flags &= ~EmojiChar;
            break;
        }
        goto push;
    default:
        goto assign;
    }

replace:
    m->len = 1;
    m->seq[0] = REPLACEMENT_CH;
    m->nextLen = 0;
    *flagsor |= m->flags;
    return m->flags ? -3 : -2;
assign:
    m->flags = gcb & PerCharFlags;
    m->len = 1;
    m->seq[0] = c;
    *flagsor |= m->flags;
    return (m->flags != 0) + 1;
push:
    m->seq[m->len] = c;
    m->len += (m->len < MAX_CLUSTER_SIZE);
    *flagsor |= m->flags;
    return 0;
}

//
// Base plugin stuff
//
static void
teardown(UnicodingImpl *m)
{
    free(const_cast<char*>(m->params.params[1]));
    delete [] m->params.params;
    delete [] m->seq;
}

static const char *
getParam(const UnicodingParams *params, const char *param)
{
    size_t n = strlen(param);
    for (const char **p = params->params; *p; ++p)
        if (!strncmp(*p, param, n))
            return *p;
    return nullptr;
}

static int32_t
create(int32_t, const UnicodingParams *params, int64_t, UnicodingImpl *m)
{
    m->version = UNIPLUGIN_VERSION;
    m->seq = new codepoint_t[MAX_CLUSTER_SIZE * 2];
    m->nextSeq = m->seq + MAX_CLUSTER_SIZE;

    m->params.variant = PLUGIN_NAME;
    m->params.params = new const char *[4]{};
    m->params.params[0] = TSQ_UNICODE_PARAM_REVISION "=" PLUGIN_REVISION;
    m->params.params[1] = strdup(getParam(params, TSQ_UNICODE_PARAM_STDLIB));

    if (getParam(params, TSQ_UNICODE_PARAM_WIDEAMBIG)) {
        m->privdata = (int64_t)&g_double_ambig_table;
        m->params.params[2] = TSQ_UNICODE_PARAM_WIDEAMBIG;
    }

    m->teardown = teardown;
    m->widthAt = widthAt;
    m->widthNext = widthNext;
    m->widthCategoryOf = widthCategoryOf;
    return 0;
}

static const UnicodingVariant s_variants[] = {
    { PLUGIN_NAME, VFPlatformDependent, NULL, create },
    { NULL }
};

extern "C" int32_t
uniplugin_syswc_init(int32_t, UnicodingInfo *info)
{
    info->variants = s_variants;
    return info->version = UNIPLUGIN_VERSION;
}

#ifdef PLUGIN
extern "C" int32_t
uniplugin_init(int32_t, UnicodingInfo*)
__attribute__((alias("uniplugin_syswc_init")));
#endif
