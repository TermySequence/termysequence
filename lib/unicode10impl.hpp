// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

// Included into unicode.cpp
#include "uniset.h"
#include "utf8.h"

#include <cstdio>

#define TEXT_SELECTOR   0xFE0E
#define EMOJI_SELECTOR  0xFE0F
#define ZWJ             0x200D

namespace Tsq_Unicode10 {
#include "unicode10tab.hpp"
using namespace Tsq;

//
// Unicode 10.0 base class
//
class Unicode10Base: public Tsq::Unicoding
{
protected:
    const Uniset &m_doublewidth;
    bool m_emoji;
    bool m_wideambig;

public:
    Unicode10Base(bool emoji, bool wideambig);

    UnicodingSpec spec() const;
};

Unicode10Base::Unicode10Base(bool emoji, bool wideambig) :
    m_doublewidth(wideambig ? s_ambigwidth : s_doublewidth),
    m_emoji(emoji),
    m_wideambig(wideambig)
{
}

UnicodingSpec
Unicode10Base::spec() const
{
    UnicodingSpec result;
    result.variant = TSQ_UNICODE_VARIANT_100;
    result.revision = TSQ_UNICODE_REVISION_100;
    result.emoji = m_emoji;
    result.wideambig = m_wideambig;
    return result;
}

//
// Set: Unicode 10.0 with Emoji
//
class Emoji final: public Unicode10Base
{
public:
    Emoji(bool wideambig = false) : Unicode10Base(true, wideambig) {}

    int widthAt(std::string::const_iterator pos,
                std::string::const_iterator end) const;
    int widthNext(std::string::const_iterator &posret,
                  std::string::const_iterator end);
    int widthCategoryOf(codepoint_t c, CellFlags &flagsor);

    void next(std::string::const_iterator &posret,
              std::string::const_iterator end);

    std::string nextEmojiName() const;
};

int
Emoji::widthAt(string::const_iterator i, string::const_iterator j) const
{
    codepoint_t c = utf8::unchecked::next(i);

    if (m_doublewidth.has(c))
        return 2;

    if (s_emoji_all.contains(c) && i != j) {
        c = utf8::unchecked::peek_next(i);
        if (c == EMOJI_SELECTOR || s_emoji_mods.has(c))
            return 2;
    }

    return 1;
}

int
Emoji::widthNext(string::const_iterator &i, string::const_iterator j)
{
    codepoint_t c = utf8::unchecked::next(i);
    int width = 1 + m_doublewidth.has(c);

    if (s_emoji_all.contains(c))
    {
        m_nextFlags = s_emoji_pres.has(c) ? EmojiChar : 0;
        m_nextSeq.assign(1, c);

        while (i != j) {
            c = utf8::unchecked::peek_next(i);

            if (s_zerowidth.has(c))
            {
                if (c == EMOJI_SELECTOR && m_nextSeq.size() == 1)
                {
                    width = 2;
                    m_nextFlags = EmojiChar;
                }
                else if (c == TEXT_SELECTOR && m_nextSeq.size() == 1)
                {
                    // Note: Reverting width to 1 isn't supported
                    m_nextFlags = 0;
                }
            }
            else if (s_emoji_mods.has(c))
            {
                width = 2;
                m_nextFlags = EmojiChar;
            }
            else if (s_emoji_flags.has(c) && m_nextSeq.size() == 1 &&
                     s_emoji_flags.has(m_nextSeq.front()))
            {
                ; // next
            }
            else if (!m_nextFlags || m_nextSeq.size() <= 1 || m_nextSeq.back() != ZWJ)
            {
                break;
            }

            m_nextSeq.push_back(c);
            utf8::unchecked::next(i);
        }
    }
    else
    {
        m_nextFlags = 0;

        while (i != j && s_zerowidth.has(utf8::unchecked::peek_next(i)))
            utf8::unchecked::next(i);
    }

    return width;
}

void
Emoji::next(string::const_iterator &i, string::const_iterator j)
{
    codepoint_t c = utf8::unchecked::next(i);
    m_nextSeq.assign(1, c);

    if (s_emoji_all.contains(c))
    {
        bool emoji = s_emoji_pres.has(c);

        while (i != j) {
            c = utf8::unchecked::peek_next(i);

            if (s_zerowidth.has(c))
            {
                if (c == EMOJI_SELECTOR && m_nextSeq.size() == 1)
                {
                    emoji = true;
                }
                else if (c == TEXT_SELECTOR && m_nextSeq.size() == 1)
                {
                    emoji = false;
                }
            }
            else if (s_emoji_mods.has(c))
            {
                emoji = true;
            }
            else if (s_emoji_flags.has(c) && m_nextSeq.size() == 1 &&
                     s_emoji_flags.has(m_nextSeq.front()))
            {
                ; // next
            }
            else if (!emoji || m_nextSeq.size() <= 1 || m_nextSeq.back() != ZWJ)
            {
                break;
            }

            m_nextSeq.push_back(utf8::unchecked::next(i));
        }
    }
    else
    {
        while (i != j && s_zerowidth.has(utf8::unchecked::peek_next(i)))
            m_nextSeq.push_back(utf8::unchecked::next(i));
    }
}

int
Emoji::widthCategoryOf(codepoint_t c, CellFlags &flags)
{
    Unirange r(c, c);
    int rc;

    if (s_zerowidth.has(r))
    {
        rc = 0;

        if (c == EMOJI_SELECTOR && m_seq.size() == 1 && s_emoji_all.contains(m_seq.front()))
        {
            rc = (m_flags & DblWidthChar) ? 0 : -2;
            m_flags = EmojiChar|DblWidthChar;
        }
        else if (c == TEXT_SELECTOR && m_seq.size() == 1)
        {
            // Note: Reverting width to 1 isn't supported
            m_flags &= ~EmojiChar;
        }

        goto push;
    }

    if (s_emoji_all.contains(r))
    {
        if (s_emoji_mods.has(r) && !m_seq.empty() && s_emoji_all.contains(m_seq.front()))
        {
            rc = (m_flags & DblWidthChar) ? 0 : -2;
            m_flags = EmojiChar|DblWidthChar;
            goto push;
        }
        if (s_emoji_flags.has(c) && m_seq.size() == 1 && s_emoji_flags.has(m_seq.front()))
        {
            rc = 0;
            goto push;
        }
        if (m_flags & EmojiChar && m_seq.size() > 1 && m_seq.back() == ZWJ)
        {
            rc = 0;
            goto push;
        }

        if (s_emoji_pres.has(r)) {
            rc = 2;
            m_flags = EmojiChar|DblWidthChar;
            goto assign;
        }
    }

    if (m_doublewidth.has(r)) {
        m_flags = DblWidthChar;
        rc = 2;
    } else {
        m_flags = 0;
        rc = 1;
    }
assign:
    m_seq.assign(1, c);
    flags |= m_flags;
    return rc;
push:
    m_seq.push_back(c);
    flags |= m_flags;
    return rc;
}

string
Emoji::nextEmojiName() const
{
    string result;
    char buf[16];

    for (unsigned i = 0, n = m_nextSeq.size(); i < n; ++i) {
        if (i == 1 && m_nextSeq[i] == EMOJI_SELECTOR)
            continue;

        snprintf(buf, sizeof(buf), "%x", m_nextSeq[i]);
        result.append(buf);
        result.push_back('-');
    }

    if (!result.empty())
        result.pop_back();

    return result;
}

//
// Set: Unicode 10.0 without Emoji
//
class Text final: public Unicode10Base
{
private:
    std::vector<codepoint_t> m_wnseq;

public:
    Text(bool wideambig = false) : Unicode10Base(false, wideambig) {}

    int widthAt(std::string::const_iterator pos,
                std::string::const_iterator end) const;
    int widthNext(std::string::const_iterator &posret,
                  std::string::const_iterator end);
    int widthCategoryOf(codepoint_t c, CellFlags &flagsor);

    void next(std::string::const_iterator &posret,
              std::string::const_iterator end);
};

int
Text::widthAt(string::const_iterator i, string::const_iterator j) const
{
    return 1 + m_doublewidth.has(utf8::unchecked::next(i));
}

int
Text::widthNext(string::const_iterator &i, string::const_iterator j)
{
    int width = 1 + m_doublewidth.has(utf8::unchecked::next(i));

    while (i != j && s_zerowidth.has(utf8::unchecked::peek_next(i)))
        utf8::unchecked::next(i);

    return width;
}

void
Text::next(string::const_iterator &i, string::const_iterator j)
{
    m_nextSeq.assign(1, utf8::unchecked::next(i));

    while (i != j && s_zerowidth.has(utf8::unchecked::peek_next(i)))
        m_nextSeq.push_back(utf8::unchecked::next(i));
}

int
Text::widthCategoryOf(codepoint_t c, CellFlags &flags)
{
    Unirange r(c, c);
    int rc;

    if (s_zerowidth.has(r))
    {
        m_seq.push_back(c);
        flags |= m_flags;
        return 0;
    }

    if (m_doublewidth.has(r)) {
        m_flags = DblWidthChar;
        rc = 2;
    } else {
        m_flags = 0;
        rc = 1;
    }

    m_seq.assign(1, c);
    flags |= m_flags;
    return rc;
}
}
