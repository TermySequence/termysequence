// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "unicode.h"
#include "uniset.h"
#include "utf8.h"
#include "config.h"

#include "unicode10tab.hpp"
using namespace Tsq;

//
// Set: Unicode 10.0 with Emoji
//
static int32_t
emoji_widthAt(const UnicodingImpl *m, const char *i, const char *j)
{
    codepoint_t c = utf8::unchecked::next(i);

    if (((const Uniset *)m->privdata)->has(c))
        return 2;

    if (s_emoji_all.contains(c) && i != j) {
        c = utf8::unchecked::peek_next(i);
        if (c == EMOJI_SELECTOR || s_emoji_mods.has(c))
            return 2;
    }

    return 1;
}

static int32_t
emoji_widthNext(UnicodingImpl *m, const char **i, const char *j)
{
    codepoint_t c = utf8::unchecked::next(*i);
    int width = 1 + ((const Uniset *)m->privdata)->has(c);

    if (s_emoji_all.contains(c))
    {
        m->nextSeq[0] = c;
        m->nextLen = 1;
        m->nextFlags = s_emoji_pres.has(c) ? EmojiChar : 0;

        while (*i != j) {
            c = utf8::unchecked::peek_next(*i);

            if (s_zerowidth.has(c))
            {
                if (c == EMOJI_SELECTOR && m->nextLen == 1)
                {
                    width = 2;
                    m->nextFlags = EmojiChar;
                }
                else if (c == TEXT_SELECTOR && m->nextLen == 1)
                {
                    // Note: Reverting width to 1 isn't supported
                    m->nextFlags = 0;
                }
            }
            else if (s_emoji_mods.has(c))
            {
                width = 2;
                m->nextFlags = EmojiChar;
            }
            else if (s_emoji_flags.has(c) && m->nextLen == 1 &&
                     s_emoji_flags.has(m->nextSeq[0]))
            {
                ; // next
            }
            else if (!m->nextFlags || m->nextLen <= 1 || m->nextSeq[m->nextLen - 1] != ZWJ)
            {
                break;
            }

            m->nextSeq[m->nextLen] = c;
            m->nextLen += (m->nextLen < MAX_CLUSTER_SIZE - 1);
            utf8::unchecked::next(*i);
        }
    }
    else
    {
        m->nextFlags = 0;

        while (*i != j && s_zerowidth.has(utf8::unchecked::peek_next(*i)))
            utf8::unchecked::next(*i);
    }

    return width;
}

static void
emoji_next(UnicodingImpl *m, const char **i, const char *j)
{
    codepoint_t c = utf8::unchecked::next(*i);
    m->nextSeq[0] = c;
    m->nextLen = 1;

    if (s_emoji_all.contains(c))
    {
        bool emoji = s_emoji_pres.has(c);

        while (*i != j) {
            c = utf8::unchecked::peek_next(*i);

            if (s_zerowidth.has(c))
            {
                if (c == EMOJI_SELECTOR && m->nextLen == 1)
                {
                    emoji = true;
                }
                else if (c == TEXT_SELECTOR && m->nextLen == 1)
                {
                    emoji = false;
                }
            }
            else if (s_emoji_mods.has(c))
            {
                emoji = true;
            }
            else if (s_emoji_flags.has(c) && m->nextLen == 1 &&
                     s_emoji_flags.has(m->nextSeq[0]))
            {
                ; // next
            }
            else if (!emoji || m->nextLen <= 1 || m->nextSeq[m->nextLen - 1] != ZWJ)
            {
                break;
            }

            m->nextSeq[m->nextLen] = utf8::unchecked::next(*i);
            m->nextLen += (m->nextLen < MAX_CLUSTER_SIZE - 1);
        }
    }
    else
    {
        while (*i != j && s_zerowidth.has(utf8::unchecked::peek_next(*i))) {
            m->nextSeq[m->nextLen] = utf8::unchecked::next(*i);
            m->nextLen += (m->nextLen < MAX_CLUSTER_SIZE - 1);
        }
    }
}

static int32_t
emoji_widthCategoryOf(UnicodingImpl *m, codepoint_t c, CellFlags *flagsor)
{
    Unirange r(c, c);
    int rc;

    if (s_zerowidth.has(r))
    {
        rc = 0;

        if (c == EMOJI_SELECTOR && m->len == 1 && s_emoji_all.contains(m->seq[0]))
        {
            rc = (m->flags & DblWidthChar) ? 0 : -2;
            m->flags = EmojiChar|DblWidthChar;
        }
        else if (c == TEXT_SELECTOR && m->len == 1)
        {
            // Note: Reverting width to 1 isn't supported
            m->flags &= ~EmojiChar;
        }

        goto push;
    }

    if (s_emoji_all.contains(r))
    {
        if (s_emoji_mods.has(r) && m->len && s_emoji_all.contains(m->seq[0]))
        {
            rc = (m->flags & DblWidthChar) ? 0 : -2;
            m->flags = EmojiChar|DblWidthChar;
            goto push;
        }
        if (s_emoji_flags.has(c) && m->len == 1 && s_emoji_flags.has(m->seq[0]))
        {
            rc = 0;
            goto push;
        }
        if (m->flags & EmojiChar && m->len > 1 && m->seq[m->len - 1] == ZWJ)
        {
            rc = 0;
            goto push;
        }

        if (s_emoji_pres.has(r)) {
            rc = 2;
            m->flags = EmojiChar|DblWidthChar;
            goto assign;
        }
    }

    if (((const Uniset *)m->privdata)->has(r)) {
        m->flags = DblWidthChar;
        rc = 2;
    } else {
        m->flags = 0;
        rc = 1;
    }
assign:
    m->seq[0] = c;
    m->len = 1;
    *flagsor |= m->flags;
    return rc;
push:
    m->seq[m->len] = c;
    m->len += (m->len < MAX_CLUSTER_SIZE - 1);
    *flagsor |= m->flags;
    return rc;
}

//
// Set: Unicode 10.0 without Emoji
//
static int32_t
text_widthAt(const UnicodingImpl *m, const char *i, const char *j)
{
    return 1 + ((const Uniset *)m->privdata)->has(utf8::unchecked::next(i));
}

static int32_t
text_widthNext(UnicodingImpl *m, const char **i, const char *j)
{
    int width = 1 + ((const Uniset *)m->privdata)->has(utf8::unchecked::next(*i));

    while (*i != j && s_zerowidth.has(utf8::unchecked::peek_next(*i)))
        utf8::unchecked::next(*i);

    return width;
}

static void
text_next(UnicodingImpl *m, const char **i, const char *j)
{
    m->nextSeq[0] = utf8::unchecked::next(*i);
    m->nextLen = 1;

    while (*i != j && s_zerowidth.has(utf8::unchecked::peek_next(*i))) {
        m->nextSeq[m->nextLen] = utf8::unchecked::next(*i);
        m->nextLen += (m->nextLen < MAX_CLUSTER_SIZE - 1);
    }
}

static int32_t
text_widthCategoryOf(UnicodingImpl *m, codepoint_t c, CellFlags *flagsor)
{
    Unirange r(c, c);
    int rc;

    if (s_zerowidth.has(r))
    {
        m->seq[m->len] = c;
        m->len += (m->len < MAX_CLUSTER_SIZE - 1);
        *flagsor |= m->flags;
        return 0;
    }

    if (((const Uniset *)m->privdata)->has(r)) {
        m->flags = DblWidthChar;
        rc = 2;
    } else {
        m->flags = 0;
        rc = 1;
    }

    m->seq[0] = c;
    m->len = 1;
    *flagsor |= m->flags;
    return rc;
}

//
// Unicode 10.0 base class
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

    m->params.variant = TSQ_UNICODE_VARIANT_100;
    m->params.revision = TSQ_UNICODE_REVISION_100;
    const int maxParams = 3;
    int curParam = 0;
    m->params.params = new const char *[maxParams]{};

    if (hasParam(params, "+" TSQ_UNICODE_PARAM_EMOJI)) {
        m->widthAt = emoji_widthAt;
        m->widthNext = emoji_widthNext;
        m->widthCategoryOf = emoji_widthCategoryOf;
        m->next = emoji_next;
        m->params.params[curParam++] = "+" TSQ_UNICODE_PARAM_EMOJI;
    } else {
        m->widthAt = text_widthAt;
        m->widthNext = text_widthNext;
        m->widthCategoryOf = text_widthCategoryOf;
        m->next = text_next;
    }

    if (hasParam(params, "+" TSQ_UNICODE_PARAM_WIDEAMBIG)) {
        m->privdata = (void*)&s_ambigwidth;
        m->params.params[curParam++] = "+" TSQ_UNICODE_PARAM_WIDEAMBIG;
    } else {
        m->privdata = (void*)&s_doublewidth;
    }

    m->teardown = teardown;
    return 0;
}

static const UnicodingVariant s_variants[] = {
    { TSQ_UNICODE_VARIANT_100, TSQ_UNICODE_REVISION_100 },
    { NULL }
};
static const char *s_params[] = {
    "+" TSQ_UNICODE_PARAM_EMOJI,
    "+" TSQ_UNICODE_PARAM_WIDEAMBIG,
    NULL
};

extern "C" int32_t
uniplugin_init(int32_t, UnicodingInfo *info)
{
    info->version = UNIPLUGIN_VERSION;
    info->variants = s_variants;
    info->params = s_params;
    info->defaultName = TSQ_UNICODE_DEFAULT;
    info->create = create;
    return 0;
}
