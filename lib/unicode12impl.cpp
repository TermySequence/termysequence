// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "unicode.h"
#include "unitable.h"
#include "utf8.h"
#include "config.h"

using namespace Tsq;

// Redefine CellFlags values for our own use
enum GcbParserFlag : uint32_t {
    GcbPictographicSequence    = 1u,
    GcbStateMask               = 255u,

    GcbZwj                     = 1u << 8,
    GcbSkinToneModifier        = 1u << 9,
    GcbHangulL                 = 1u << 10,
    GcbHangulV                 = 1u << 11,
    GcbHangulT                 = 1u << 12,
    GcbHangulLV                = 1u << 13,
    GcbHangulLVT               = 1u << 14,
    GcbFlagsMask               = 255u << 8,

    GcbCombining               = 1u << 16,
    GcbPictographic            = 1u << 17,
    GcbEmojiModifier           = 1u << 18,
    GcbTextModifier            = 1u << 19,
    GcbRegionalIndicator       = 1u << 20,
    GcbHangul                  = 1u << 21,
    GcbBaseMask                = 255u << 16,

    // EmojiChar               = 1u << 30,
    // DblWidthChar            = 1u << 31,
    // PerCharFlags            = 3u << 30,

    GcbPictographicJoin        = (GcbPictographicSequence|GcbZwj),
};

#include "unicode12tab.hpp"

static const Unitable<codepoint_t, 0>
s_single_ambig_table(s_single_ambig_data, ARRAY_SIZE(s_single_ambig_data) / 3);

static const Unitable<codepoint_t, 0>
s_double_ambig_table(s_double_ambig_data, ARRAY_SIZE(s_double_ambig_data) / 3);

static const Unitable<uint16_t, GcbHangulLVT>
s_hangul_table(s_hangul_data, ARRAY_SIZE(s_hangul_data) / 3);

typedef typeof(s_single_ambig_table) *MainTablePtr;

//
// TermyUnicode methods
//
static bool
hangul_combines(codepoint_t a, codepoint_t b)
{
    CellFlags a_gcb = s_hangul_table.search(a);
    CellFlags b_gcb = s_hangul_table.search(b);

    if (a_gcb & GcbHangulL)
        return b_gcb & (GcbHangulL|GcbHangulV|GcbHangulLV|GcbHangulLVT);

    if (a_gcb & (GcbHangulLV|GcbHangulV))
        return b_gcb & (GcbHangulV|GcbHangulT);

    if (a_gcb & (GcbHangulLVT|GcbHangulT))
        return b_gcb & GcbHangulT;

    return false;
}

static inline bool
emoji_combines(CellFlags a_gcb, CellFlags b_gcb)
{
    return !(b_gcb & GcbSkinToneModifier) || (a_gcb & GcbPictographic);
}

static int32_t
widthAt(const UnicodingImpl *m, const char *i, const char *j)
{
    auto table = (MainTablePtr)m->privdata;
    CellFlags gcb = table->lookup(utf8::unchecked::next(i));

    if (gcb & DblWidthChar)
        return 2;

    if (i != j)
        if (emoji_combines(gcb, table->lookup(utf8::unchecked::peek_next(i))))
            return 2;

    return 1;
}

static int32_t
widthNext(UnicodingImpl *m, const char **i, const char *j)
{
    auto table = (MainTablePtr)m->privdata;
    codepoint_t c = utf8::unchecked::next(*i);
    CellFlags gcb = table->lookup(c);
    bool is_pictoseq = gcb & GcbPictographic;
    m->nextFlags = gcb & PerCharFlags;
    m->nextLen = 1;
    m->nextSeq[0] = c;

    while (*i != j) {
        auto k = *i;
        codepoint_t next_c = utf8::unchecked::next(k);
        CellFlags next_gcb = table->lookup(next_c);

        switch (next_gcb & GcbBaseMask) {
        case GcbEmojiModifier:
            if (emoji_combines(gcb, next_gcb)) {
                // Upgrade width
                // Add emoji flag
                m->nextFlags = EmojiChar|DblWidthChar;
                break;
            } else {
                goto out;
            }
        case GcbTextModifier:
            is_pictoseq = false;
            // Note: Reverting width to 1 isn't supported
            m->nextFlags &= ~EmojiChar;
            break;
        case GcbPictographic:
            if (is_pictoseq && gcb & GcbZwj)
                break;
            else
                goto out;
        case GcbRegionalIndicator:
            if (gcb & GcbRegionalIndicator)
                break;
            else
                goto out;
        case GcbHangul:
            if (hangul_combines(m->nextSeq[m->nextLen - 1], next_c))
                break;
            else
                goto out;
        case GcbCombining:
            break;
        default:
            goto out;
        }

        gcb = next_gcb;
        *i = k;
        m->nextSeq[m->nextLen] = next_c;
        m->nextLen += (m->nextLen < MAX_CLUSTER_SIZE);
    }
out:
    return (m->nextFlags != 0) + 1;
}

static int32_t
widthCategoryOf(UnicodingImpl *m, codepoint_t c, CellFlags *flagsor)
{
    auto table = (MainTablePtr)m->privdata;
    CellFlags gcb = table->search(c);
    int rc;

    if (m->len == 0)
        goto assign;

    rc = 0;

    switch (gcb & GcbBaseMask) {
    case GcbEmojiModifier:
        if (emoji_combines(m->state, gcb)) {
            // Upgrade width
            if (!m->flags)
                rc = -2;
            // Add emoji flag
            m->flags = EmojiChar|DblWidthChar;
            goto push;
        } else {
            goto assign;
        }
    case GcbTextModifier:
        m->state &= ~GcbPictographicSequence;
        // Note: Reverting width to 1 isn't supported
        m->flags &= ~EmojiChar;
        goto push;
    case GcbPictographic:
        if ((m->state & GcbPictographicJoin) == GcbPictographicJoin) {
            goto push;
        } else {
            gcb |= GcbPictographicSequence;
            goto assign;
        }
    case GcbRegionalIndicator:
        if (m->state & GcbRegionalIndicator)
            goto push;
        else
            goto assign;
    case GcbHangul:
        if (hangul_combines(m->seq[m->len - 1], c))
            goto push;
        else
            goto assign;
    case GcbCombining:
        goto push;
    default:
        goto assign;
    }

assign:
    m->state = gcb;
    m->flags = gcb & PerCharFlags;
    m->len = 1;
    m->seq[0] = c;
    *flagsor |= m->flags;
    return (m->flags != 0) + 1;
push:
    m->state = (m->state & GcbStateMask) | gcb;
    m->seq[m->len] = c;
    m->len += (m->len < MAX_CLUSTER_SIZE);
    *flagsor |= m->flags;
    return rc;
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

static bool
hasParam(const UnicodingParams *params, const char *param)
{
    const char **p = params->params;
    while (*p)
        if (!strcmp(*p++, param))
            return true;
    return false;
}

static int32_t
create(int32_t, const UnicodingParams *params, UnicodingImpl *m)
{
    m->version = UNIPLUGIN_VERSION;
    m->seq = new codepoint_t[MAX_CLUSTER_SIZE * 2];
    m->nextSeq = m->seq + MAX_CLUSTER_SIZE;

    m->params.variant = TSQ_UNICODE_VARIANT_TERMY;
    m->params.params = new const char *[3]{};
    m->params.params[0] = TSQ_UNICODE_PARAM_REVISION "=" TSQ_UNICODE_REVISION_TERMY;

    if (hasParam(params, TSQ_UNICODE_PARAM_WIDEAMBIG)) {
        m->privdata = (void*)&s_double_ambig_table;
        m->params.params[1] = TSQ_UNICODE_PARAM_WIDEAMBIG;
    } else {
        m->privdata = (void*)&s_single_ambig_table;
    }

    m->teardown = teardown;
    m->widthAt = widthAt;
    m->widthNext = widthNext;
    m->widthCategoryOf = widthCategoryOf;
    return 0;
}

static const char *s_params[] = {
    TSQ_UNICODE_PARAM_WIDEAMBIG,
    NULL
};
static const UnicodingVariant s_variants[] = {
    { "", VFSelectable, s_params, create },
    { NULL }
};

extern "C" int32_t
uniplugin_init(int32_t, UnicodingInfo *info)
{
    info->variants = s_variants;
    return info->version = UNIPLUGIN_VERSION;
}
