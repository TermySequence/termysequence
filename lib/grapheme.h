// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "unicode.h"

namespace Tsq
{
    //
    // Simple grapheme cluster iterator
    //
    class GraphemeWalk
    {
    protected:
        Unicoding *m_coding;
        const std::string &m_str;
        std::string::const_iterator m_cur, m_next, m_end;

    public:
        GraphemeWalk(Unicoding *coding, const std::string &str);

        inline size_t start() const { return m_cur - m_str.begin(); }
        inline size_t end() const { return m_next - m_str.begin(); }

        inline bool finished() const { return m_next == m_end; }

        bool next();
        codepoint_t codepoint() const;

        inline std::string getEmojiName() const;
    };

    inline std::string
    GraphemeWalk::getEmojiName() const
    {
        return m_coding->nextEmojiName();
    }

    //
    // Reports individual double-width characters and emoji
    //
    struct DoubleWidthWalk: GraphemeWalk
    {
        DoubleWidthWalk(Unicoding *coding, const std::string &str);
        column_t next(CellFlags &flagsret);
    };

    //
    // Width and emoji substring iterator
    //
    struct CategoryWalk: GraphemeWalk
    {
        CategoryWalk(Unicoding *coding, const std::string &str);
        column_t next(CellFlags &flagsret);
    };

    //
    // Emoji-only substring iterator
    //
    struct EmojiWalk: GraphemeWalk
    {
        EmojiWalk(Unicoding *coding, const std::string &str);
        bool next(CellFlags &flagsret);
    };
}
