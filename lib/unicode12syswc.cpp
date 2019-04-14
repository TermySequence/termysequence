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
// SystemWcWidth methods
//
static Tsq::CellFlags
syswc_lookup(codepoint_t c)
{
    switch(wcwidth(c)) {
    case -1:
        return GcbInvalid;
    case 0:
        return GcbCombining;
    case 1:
        return 0;
    default:
        return DblWidthChar | (g_double_ambig_table.lookup(c) & EmojiChar);
    }
}

static int32_t
widthAt(const UnicodingImpl *m, const char *i, const char *j)
{
    codepoint_t c = utf8::unchecked::next(i);
    return 1 + (wcwidth(c) > 1);
}

static int32_t
widthNext(UnicodingImpl *m, const char **i, const char *j)
{
    codepoint_t c = utf8::unchecked::next(*i);
    Tsq::CellFlags gcb = syswc_lookup(c);

    m->nextFlags = gcb & PerCharFlags;
    m->nextLen = 1;
    m->nextSeq[0] = c;

    while (*i != j) {
        auto k = *i;
        c = utf8::unchecked::next(k);

        if (!(syswc_lookup(c) & GcbCombining))
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
    Tsq::CellFlags gcb = syswc_lookup(c);

    if (m->len == 0)
        goto assign;

    switch (gcb & GcbBaseMask) {
    case GcbInvalid:
        goto replace;
    case GcbCombining:
        switch (c) {
        case EMOJI_SELECTOR:
            if (m->nextFlags == DblWidthChar)
                m->nextFlags |= EmojiChar;
            break;
        case TEXT_SELECTOR:
            m->nextFlags &= ~EmojiChar;
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
    *flagsor |= (m->privdata & PerCharFlags);
    return (int16_t)m->privdata;
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
    delete [] m->params.params;
    delete [] m->seq;
}

static int32_t
create(int32_t, const UnicodingParams *params, int64_t, UnicodingImpl *m)
{
    m->version = UNIPLUGIN_VERSION;
    m->seq = new codepoint_t[MAX_CLUSTER_SIZE * 2];
    m->nextSeq = m->seq + MAX_CLUSTER_SIZE;

    m->params.variant = PLUGIN_NAME;
    m->params.params = new const char *[2]{};
    m->params.params[0] = TSQ_UNICODE_PARAM_REVISION "=" PLUGIN_REVISION;

    m->privdata = wcwidth(REPLACEMENT_CH) > 1 ? DblWidthChar|-2 : ~DblWidthChar&-3;

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
