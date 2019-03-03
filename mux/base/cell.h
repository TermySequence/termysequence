// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"
#include "lib/flags.h"
#include "cursor.h"
#include "color.h"

#include <vector>

namespace Tsq { class Unicoding; }

struct CellAttributes
{
    Tsq::CellFlags flags;
    Color fg;
    Color bg;
    regionid_t link;

    inline CellAttributes() :
        flags(0), fg(0), bg(0), link(INVALID_REGION_ID)
        {}
    inline CellAttributes(const uint32_t *ptr) :
        flags(ptr[0]), fg(ptr[1]), bg(ptr[2]), link(ptr[3])
        {}

    inline bool operator==(const uint32_t *ptr) const {
        return ptr[0] == flags && ptr[1] == fg && ptr[2] == bg && ptr[3] == link;
    }
};

class CellRow
{
    friend struct TermEventTransfer;
    friend class TermReader;

private:
    std::string m_str;
    std::vector<uint32_t> m_ranges;
    uint32_t m_clusters;
    int32_t m_columns;

    void coalesceRanges(size_t loc);
    void deleteRange(size_t loc);
    void insertRange(size_t loc, const CellAttributes &a, unsigned pos);
    void splitRangeAround(size_t loc, unsigned pos);
    void updateRanges(unsigned pos, const CellAttributes &a);

    size_t splitChar(const char *i, unsigned pos, Tsq::Unicoding *lookup);
    void removeChar(const char *i, unsigned pos, Tsq::Unicoding *lookup);
    void mergeChars(const char *i, unsigned pos, Tsq::Unicoding *lookup);

public:
    Tsq::LineFlags flags;
    int32_t modtime;

    inline CellRow() :
        m_clusters(0), m_columns(0), flags(0), modtime(INVALID_MODTIME)
    {}

    inline const std::string& str() const { return m_str; }
    inline int32_t columns() const { return m_columns; }
    inline bool isEmpty() const { return m_clusters == 0; }
    inline uint32_t numRanges() const { return m_ranges.size() / 6; }

    std::string substr(unsigned startPos, Tsq::Unicoding *lookup) const;
    std::string substr(unsigned startPos, unsigned endPos,
                       Tsq::Unicoding *lookup) const;

    void updateCursor(Cursor &cursor, Tsq::Unicoding *lookup) const;

    void pad(unsigned n);
    void combine(Cursor &cursor, const CellAttributes &a, codepoint_t c);
    size_t append(const CellAttributes &a, codepoint_t c, int width);
    size_t replace(Cursor &cursor, const CellAttributes &a, codepoint_t c,
                   int width, Tsq::Unicoding *lookup);

    void insert(int x, Tsq::Unicoding *lookup);
    void remove(int x, Tsq::Unicoding *lookup);
    void resize(int x, Tsq::Unicoding *lookup);

    void selectiveErase(int startx, int endx, Tsq::Unicoding *lookup);
    void selectiveErase(int startx, Tsq::Unicoding *lookup);
    void erase(int startx, int endx, Tsq::Unicoding *lookup);
    void erase();

    void clear();
};

inline void
CellRow::erase()
{
    m_str.clear();
    m_ranges.clear();

    m_clusters = m_columns = 0;
    flags = Tsq::NoLineFlags;
}

inline void
CellRow::clear()
{
    erase();
    modtime = INVALID_MODTIME;
}
