// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "grapheme.h"
#include "utf8.h"

namespace Tsq
{
    //
    // Simple grapheme cluster iterator
    //
    GraphemeWalk::GraphemeWalk(Unicoding *coding, const std::string &str) :
        m_coding(coding),
        m_str(str),
        m_next(str.data()),
        m_end(m_next + str.size())
    {
    }

    bool
    GraphemeWalk::next()
    {
        if (m_next == m_end)
            return false;

        m_cur = m_next;
        m_coding->widthNext(m_next, m_end);
        return true;
    }

    codepoint_t
    GraphemeWalk::codepoint() const
    {
        return utf8::unchecked::peek_next(m_cur);
    }

    //
    // Reports individual double-width characters and emoji
    //
    DoubleWidthWalk::DoubleWidthWalk(Unicoding *coding, const std::string &str) :
        GraphemeWalk(coding, str)
    {
    }

    column_t
    DoubleWidthWalk::next(CellFlags &flagsret)
    {
        if (m_next == m_end)
            return false;

        column_t size = 1;
        m_cur = m_next;
        m_coding->widthNext(m_next, m_end);
        flagsret = m_coding->nextFlags();

        if (!flagsret) {
            while (m_next != m_end) {
                const char *save = m_next;
                m_coding->widthNext(m_next, m_end);
                if (m_coding->nextFlags()) {
                    m_next = save;
                    break;
                }
               ++size;
            }
        }

        return size;
    }

    //
    // Width and emoji substring iterator
    //
    CategoryWalk::CategoryWalk(Unicoding *coding, const std::string &str) :
        GraphemeWalk(coding, str)
    {
    }

    column_t
    CategoryWalk::next(CellFlags &flagsret)
    {
        if (m_next == m_end)
            return 0;

        m_cur = m_next;

        column_t size = 1;
        m_coding->widthNext(m_next, m_end);
        flagsret = m_coding->nextFlags();

        if (!(flagsret & Tsq::EmojiChar)) {
            while (m_next != m_end) {
                const char *save = m_next;
                m_coding->widthNext(m_next, m_end);
                if (m_coding->nextFlags() != flagsret) {
                    m_next = save;
                    break;
                }
                ++size;
            }
        }

        return size;
    }

    //
    // Emoji-only substring iterator
    //
    EmojiWalk::EmojiWalk(Unicoding *coding, const std::string &str) :
        GraphemeWalk(coding, str)
    {
    }

    bool
    EmojiWalk::next(CellFlags &flagsret)
    {
        if (m_next == m_end)
            return false;

        m_cur = m_next;

        m_coding->widthNext(m_next, m_end);
        flagsret = m_coding->nextFlags();

        if (!flagsret) {
            while (m_next != m_end) {
                const char *save = m_next;
                m_coding->widthNext(m_next, m_end);
                if (m_coding->nextFlags()) {
                    m_next = save;
                    break;
                }
            }
        }

        return true;
    }
}
