// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cell.h"
#include "lib/unicode.h"
#include "lib/utf8.h"

#define FILL_BYTE ' '

#define RANGE_SIZE 6
#define RANGE_START (range)[0]
#define RANGE_NEXT_START (range + RANGE_SIZE)[0]
#define RANGE_END (range)[1]
#define RANGE_NEXT_END (range + RANGE_SIZE)[1]
#define RANGE_FLAGS (range)[2]
#define RANGE_NEXT_FLAGS (range + RANGE_SIZE)[2]
#define RANGE_FG (range)[3]
#define RANGE_NEXT_FG (range + RANGE_SIZE)[3]
#define RANGE_BG (range)[4]
#define RANGE_NEXT_BG (range + RANGE_SIZE)[4]
#define RANGE_LINK (range)[5]
#define RANGE_NEXT_LINK (range + RANGE_SIZE)[5]
#define RANGE_ATTR (range + 2)
#define RANGE_NEXT_ATTR (range + RANGE_SIZE + 2)

#define FOR_RANGES(rangevec) \
    uint32_t *range; \
    size_t loc, _rn; \
    for (range = rangevec.data(), _rn = rangevec.size(), loc = 0; \
         loc < _rn; loc += RANGE_SIZE, range += RANGE_SIZE)

#define FOR_RANGES_AGAIN(rangevec) \
    for (range = rangevec.data(), _rn = rangevec.size(), loc = 0; \
         loc < _rn; loc += RANGE_SIZE, range += RANGE_SIZE)

#define FOR_RANGES_DYN(rangevec) \
    uint32_t *range; \
    size_t loc; \
    for (range = rangevec.data(), loc = 0; loc < rangevec.size(); \
         loc += RANGE_SIZE, range += RANGE_SIZE)

/*
 * Helpers
 */
inline void
CellRow::deleteRange(size_t loc)
{
    auto i = m_ranges.begin() + loc;
    m_ranges.erase(i, i + RANGE_SIZE);
}

void
CellRow::coalesceRanges(size_t loc)
{
    // Check the range after us
    if (m_ranges.size() > loc + RANGE_SIZE) {
        auto range = m_ranges.data() + loc;

        if (RANGE_END + 1 == RANGE_NEXT_START && !memcmp(RANGE_ATTR, RANGE_NEXT_ATTR, 16))
        {
            RANGE_END = RANGE_NEXT_END;
            deleteRange(loc + RANGE_SIZE);
        }
    }
    // Check the range before us
    if (loc > 0) {
        auto range = m_ranges.data() + (loc - RANGE_SIZE);

        if (RANGE_END + 1 == RANGE_NEXT_START && !memcmp(RANGE_ATTR, RANGE_NEXT_ATTR, 16))
        {
            RANGE_END = RANGE_NEXT_END;
            deleteRange(loc);
        }
    }
}

void
CellRow::insertRange(size_t loc, const CellAttributes &a, unsigned pos)
{
    m_ranges.insert(m_ranges.begin() + loc, RANGE_SIZE, 0);

    auto range = m_ranges.data() + loc;
    RANGE_START = RANGE_END = pos;
    RANGE_FLAGS = a.flags;
    RANGE_FG = a.fg;
    RANGE_BG = a.bg;
    RANGE_LINK = a.link;
}

void
CellRow::splitRangeAround(size_t loc, unsigned pos)
{
    m_ranges.insert(m_ranges.begin() + (loc + RANGE_SIZE), RANGE_SIZE, 0);

    auto range = m_ranges.data() + loc;
    RANGE_NEXT_START = pos + 1;
    RANGE_NEXT_END = RANGE_END;
    RANGE_END = pos - 1;

    RANGE_NEXT_FLAGS = RANGE_FLAGS;
    RANGE_NEXT_FG = RANGE_FG;
    RANGE_NEXT_BG = RANGE_BG;
    RANGE_NEXT_LINK = RANGE_LINK;
}

void
CellRow::updateRanges(unsigned pos, const CellAttributes &a)
{
    // Find range that includes pos

    // There is a range:
    //    Does new cell fit in range?
    //      y -> work done
    //    Is cell the entire range?
    //      y -> remove range, a -> insert range at pos, check coalesce both ends
    //    Is cell at beginning of range?
    //      y -> shorten range, a -> insert range at pos, check coalesce prior
    //    Is cell at end of range?
    //      y -> shorten range, a -> insert range at pos, check coalesce following
    //    Otherwise -> split range around pos
    //      a -> insert range at pos, no coalesce required

    // Between ranges:
    //    a -> new range, check coalesce at both ends

    // At end:
    //    a -> new range, check coalesce prior

    FOR_RANGES(m_ranges)
    {
        if (RANGE_START > pos)
            goto betweenRanges;
        if (RANGE_END >= pos)
            goto withinRange;
    }
    if (a.flags) {
        m_ranges.push_back(pos);
        m_ranges.push_back(pos);
        m_ranges.push_back(a.flags);
        m_ranges.push_back(a.fg);
        m_ranges.push_back(a.bg);
        m_ranges.push_back(a.link);
        coalesceRanges(loc);
    }
    return;

withinRange:
    if (a == RANGE_ATTR) {
        return;
    }
    else if (RANGE_START == RANGE_END) {
        if (a.flags) {
            RANGE_FLAGS = a.flags;
            RANGE_FG = a.fg;
            RANGE_BG = a.bg;
            RANGE_LINK = a.link;
        }
        else {
            deleteRange(loc);
        }
        coalesceRanges(loc);
    }
    else if (pos == RANGE_START) {
        ++RANGE_START;
        if (a.flags) {
            insertRange(loc, a, pos);
            coalesceRanges(loc);
        }
    }
    else if (pos == RANGE_END) {
        --RANGE_END;
        if (a.flags) {
            insertRange(loc + RANGE_SIZE, a, pos);
            coalesceRanges(loc + RANGE_SIZE);
        }
    }
    else {
        splitRangeAround(loc, pos);
        if (a.flags) {
            insertRange(loc + RANGE_SIZE, a, pos);
        }
    }
    return;

betweenRanges:
    if (a.flags) {
        insertRange(loc, a, pos);
        coalesceRanges(loc);
    }
}

size_t
CellRow::splitChar(std::string::const_iterator i, unsigned pos, Tsq::Unicoding *lookup)
{
    // Remove character flags
    FOR_RANGES(m_ranges)
    {
        if (RANGE_END >= pos) {
            CellAttributes a(RANGE_ATTR);
            a.flags &= ~Tsq::PerCharFlags;
            updateRanges(pos, a);
            break;
        }
    }
    // Extend ranges
    FOR_RANGES_AGAIN(m_ranges)
    {
        if (RANGE_START > pos) {
            ++RANGE_START;
            ++RANGE_END;
        }
        else if (RANGE_END >= pos) {
            ++RANGE_END;
        }
    }

    // Find the end of the cluster
    std::string::const_iterator k = i;
    lookup->widthNext(k, m_str.end());

    // Perform the replace
    size_t retval = i - m_str.begin();
    m_str.replace(i, k, 2, FILL_BYTE);
    ++m_clusters;

    // Return the position where replacement happened
    return retval;
}

void
CellRow::removeChar(std::string::const_iterator i, unsigned pos, Tsq::Unicoding *lookup)
{
    // Find the end of the cluster
    std::string::const_iterator k = i;
    lookup->widthNext(k, m_str.end());

    // Perform the removal
    m_str.erase(i, k);
    --m_clusters;
    // Caller must update columns

    // Find range that includes pos

    // There is a range:
    //    Is cell the entire range?
    //      y -> remove range
    //      n -> shorten range by 1

    // Move all further ranges 1 to the left
    // Check coalesce at pos
    size_t cr = 0;

    FOR_RANGES_DYN(m_ranges)
    {
        if (RANGE_START > pos) {
            --RANGE_START;
            --RANGE_END;

            if (pos && loc && RANGE_START == pos)
                cr = loc;
        }
        else if (RANGE_START == pos && RANGE_END == pos) {
            deleteRange(loc);
            loc -= RANGE_SIZE;
            range = m_ranges.data() + loc;
        }
        else if (RANGE_END >= pos) {
            --RANGE_END;
        }
    }

    if (cr)
        coalesceRanges(cr);
}

void
CellRow::mergeChars(std::string::const_iterator i, unsigned pos, Tsq::Unicoding *lookup)
{
    // Find the next cluster
    std::string::const_iterator k = i, j = m_str.end();
    lookup->widthNext(k, j);

    // Case 1: End of string
    if (k == j) {
        ++m_columns;
        return;
    }
    i = k;
    ++pos;
    // Case 2: Remove single-width cluster
    if (lookup->widthNext(k, j) == 1) {
        removeChar(i, pos, lookup);
    }
    // Case 3: Replace double-width cluster with space
    else {
        // Remove character flags
        FOR_RANGES(m_ranges)
        {
            if (RANGE_END >= pos) {
                CellAttributes a(RANGE_ATTR);
                a.flags &= ~Tsq::PerCharFlags;
                updateRanges(pos, a);
                break;
            }
        }

        m_str.replace(i, k, 1, FILL_BYTE);
    }
}

/*
 * Primary
 */
void
CellRow::updateCursor(Cursor &cursor, Tsq::Unicoding *lookup) const
{
    std::string::const_iterator i = m_str.begin(), j = m_str.end();

    cursor.reset();
    int x = cursor.x();
    unsigned pos = 0;

    while (i != j) {
        auto k = i;
        int width = lookup->widthNext(k, j);

        if (x < width) {
            if (x)
                cursor.setDoubleRight();
            else if (width == 2)
                cursor.setDoubleLeft();
            goto out;
        }

        i = k;
        x -= width;
        ++pos;
    }
    pos += x;
out:
    cursor.rpos() = pos;
    cursor.rptr() = i - m_str.begin();
}

void
CellRow::pad(unsigned n)
{
    m_str.append(n, FILL_BYTE);
    m_clusters += n;
    m_columns += n;
}

void
CellRow::combine(Cursor &cursor, const CellAttributes &a, codepoint_t c)
{
    // Insert zero-width (combining) codepoint
    char buf[8];
    size_t n = utf8::unchecked::append(c, buf) - buf;
    m_str.insert(cursor.ptr(), buf, n);
    cursor.rptr() += n;

    // Adjust ranges
    updateRanges(cursor.savedPos(), a);
}

size_t
CellRow::append(const CellAttributes &a, codepoint_t c, int width)
{
    // Positive-width append

    utf8::unchecked::append(c, std::back_inserter(m_str));

    // Look at last range

    // Does new codepoint fit in range?
    //    y -> extend range
    //    n -> a -> make new range, no coalesce required

    if (!m_ranges.empty())
    {
        // Get the last range
        auto range = m_ranges.data() + (m_ranges.size() - RANGE_SIZE);

        // Should new codepoint be included?
        if (m_clusters - 1 == RANGE_END && a == RANGE_ATTR) {
            ++RANGE_END;
            goto out;
        }
    }
    if (a.flags) {
        // Start a new range?
        m_ranges.push_back(m_clusters);
        m_ranges.push_back(m_clusters);
        m_ranges.push_back(a.flags);
        m_ranges.push_back(a.fg);
        m_ranges.push_back(a.bg);
        m_ranges.push_back(a.link);
    }
out:
    ++m_clusters;
    m_columns += width;

    return m_str.size();
}

void
CellRow::insert(int x, Tsq::Unicoding *lookup)
{
    // Find the position to perform the insertion
    Cursor cursor(x);
    updateCursor(cursor, lookup);

    std::string::const_iterator i = m_str.begin() + cursor.ptr();

    if (cursor.flags() & Tsq::OnDoubleRight) {
        // Break up a double-width cluster (unaligned)
        size_t ptr = splitChar(i, cursor.pos(), lookup);
        // Move the cursor and iterator to the second space
        i = m_str.begin() + ptr + 1;
        ++cursor.rpos();
    }

    // Perform the insert
    m_str.insert(i, 1, FILL_BYTE);
    ++m_columns;
    ++m_clusters;

    // Find range that includes pos

    // There is a range:
    //     Is pos not the starting cluster?
    //       y -> extend the range by 1, split range around pos

    // Move all further ranges 1 to the right

    FOR_RANGES_DYN(m_ranges)
    {
        if (RANGE_START >= cursor.pos()) {
            ++RANGE_START;
            ++RANGE_END;
        }
        else if (RANGE_END >= cursor.pos()) {
            ++RANGE_END;
            splitRangeAround(loc, cursor.pos());
            loc += RANGE_SIZE;
            range = m_ranges.data() + loc;
        }
    }
}

void
CellRow::remove(int x, Tsq::Unicoding *lookup)
{
    // Find the position to perform the removal
    Cursor cursor(x);
    updateCursor(cursor, lookup);

    std::string::const_iterator i = m_str.begin() + cursor.ptr();

    if (cursor.flags() & (Tsq::OnDoubleLeft|Tsq::OnDoubleRight)) {
        // Break up a double-width cluster
        size_t ptr = splitChar(i, cursor.pos(), lookup);
        i = m_str.begin() + ptr;

        if (cursor.flags() & Tsq::OnDoubleRight) {
            // Move the cursor and iterator to the second space
            ++cursor.rpos();
            ++i;
        }
    }

    // Perform the removal
    removeChar(i, cursor.pos(), lookup);
    --m_columns;
}

size_t
CellRow::replace(Cursor &cursor, const CellAttributes &a, codepoint_t c,
                 int width, Tsq::Unicoding *lookup)
{
    // Get the starting pointer
    std::string::const_iterator i = m_str.begin() + cursor.ptr();
    int oldwidth = 1;

    // Merge and split clusters for double-width operations
    if (cursor.flags() & Tsq::OnDoubleRight) {
        // Break up double-width cluster (unaligned)
        splitChar(i, cursor.pos(), lookup);
        // Move the cursor and iterator to the second space
        ++cursor.rpos();
        ++cursor.rptr();
        i = m_str.begin() + cursor.ptr();
    } else if (cursor.flags() & Tsq::OnDoubleLeft) {
        oldwidth = 2;
    }

    if (oldwidth != width) {
        if (oldwidth > width) {
            // Break up double-width cluster (aligned)
            splitChar(i, cursor.pos(), lookup);
        } else {
            // Merge clusters
            mergeChars(i, cursor.pos(), lookup);
        }
        // Restore iterator
        i = m_str.begin() + cursor.ptr();
    }

    // Locate the ending pointer
    std::string::const_iterator k = i, j = m_str.end();
    lookup->widthNext(k, j);

    // Perform the replace
    char buf[8];
    size_t n = utf8::unchecked::append(c, buf) - buf;
    m_str.replace(i, k, buf, n);

    // Adjust ranges
    updateRanges(cursor.pos(), a);

    return cursor.ptr() + n;
}

void
CellRow::resize(int x, Tsq::Unicoding *lookup)
{
    if (m_columns <= x)
        return;
    m_columns = x;

    // Find the starting pointer
    Cursor cursor(x);
    updateCursor(cursor, lookup);

    size_t startptr = cursor.ptr();
    unsigned startpos = cursor.pos();

    if (cursor.flags() & Tsq::OnDoubleRight) {
        // Break up a double-width cluster
        startptr = splitChar(m_str.begin() + startptr, startpos, lookup);
        // Move the cursor and iterator to the second space
        ++startptr;
        ++startpos;
    }

    // Perform the resize
    m_str.resize(startptr);
    m_clusters = startpos;

    // Adjust ranges
    FOR_RANGES_DYN(m_ranges)
    {
        if (RANGE_START >= startpos) {
            // Delete range
            deleteRange(loc);
            loc -= RANGE_SIZE;
            range = m_ranges.data() + loc;
        }
        else if (RANGE_END >= startpos) {
            // Truncate range at end
            RANGE_END = startpos - 1;
        }
    }
}

void
CellRow::erase(int startx, int endx, Tsq::Unicoding *lookup)
{
    if (endx > m_columns)
        endx = m_columns;
    if (startx >= endx)
        return;

    // Find the starting position
    Cursor cursor(startx);
    updateCursor(cursor, lookup);

    size_t startptr = cursor.ptr();
    unsigned startpos = cursor.pos();

    if (cursor.flags() & Tsq::OnDoubleRight) {
        // Break up a double-width cluster
        startptr = splitChar(m_str.begin() + startptr, startpos, lookup);
        // Move the cursor and iterator to the second space
        ++startptr;
        ++startpos;
    }

    // Find the ending position
    cursor.setX(endx);
    updateCursor(cursor, lookup);

    std::string::const_iterator j = m_str.begin() + cursor.ptr();
    unsigned endpos = cursor.pos();

    if (cursor.flags() & Tsq::OnDoubleRight) {
        // Break up a double-width cluster
        size_t ptr = splitChar(j, endpos, lookup);
        // Move the cursor and iterator to the second space
        j = m_str.begin() + ptr + 1;
        ++endpos;
    }

    // Perform the replacement
    m_str.replace(m_str.begin() + startptr, j, endx - startx, FILL_BYTE);
    m_clusters += (endx - startx) - (endpos - startpos);

    // Adjust ranges
    FOR_RANGES_DYN(m_ranges)
    {
        if (RANGE_START >= endpos) {
            // Finished
            break;
        }
        else if (RANGE_START < startpos) {
            if (RANGE_END >= endpos) {
                // Break range in two, and finished
                m_ranges.insert(m_ranges.begin() + loc + 1, RANGE_SIZE, 0);
                range = m_ranges.data() + loc;
                RANGE_END = startpos - 1;
                RANGE_FLAGS = RANGE_NEXT_FLAGS;
                RANGE_FG = RANGE_NEXT_FG;
                RANGE_BG = RANGE_NEXT_BG;
                RANGE_LINK = RANGE_NEXT_LINK;
                RANGE_NEXT_START = endpos;
                break;
            }
            else if (RANGE_END >= startpos) {
                // Truncate range at end
                RANGE_END = startpos - 1;
            }
        }
        else if (RANGE_END >= endpos) {
            // Truncate range at start
            RANGE_START = endpos;
        }
        else {
            // Delete range
            deleteRange(loc);
            loc -= RANGE_SIZE;
            range = m_ranges.data() + loc;
        }
    }
}

void
CellRow::selectiveErase(int startx, int endx, Tsq::Unicoding *lookup)
{
    if (endx > m_columns)
        endx = m_columns;

    // Simple but inefficient implementation
    for(; startx < endx; ++startx)
    {
        Cursor cursor(startx);
        updateCursor(cursor, lookup);
        unsigned pos = cursor.pos();
        bool protect = false;

        FOR_RANGES(m_ranges) {
            if (RANGE_START > pos) {
                break;
            }
            if (RANGE_END >= pos) {
                protect = RANGE_FLAGS & Tsq::Protected;
                break;
            }
        }

        if (!protect)
            erase(startx, startx + 1, lookup);
    }
}

void
CellRow::selectiveErase(int startx, Tsq::Unicoding *lookup)
{
    selectiveErase(startx, m_columns, lookup);
}

std::string
CellRow::substr(unsigned startPos, unsigned endPos, Tsq::Unicoding *lookup) const
{
    std::string::const_iterator i = m_str.begin(), j = m_str.end();
    unsigned pos = 0;

    while (i != j && pos < startPos) {
        lookup->widthNext(i, j);
        ++pos;
    }

    std::string::const_iterator k = i;

    while (k != j && pos < endPos) {
        lookup->widthNext(k, j);
        ++pos;
    }

    return std::string(i, k);
}

std::string
CellRow::substr(unsigned startPos, Tsq::Unicoding *lookup) const
{
    std::string::const_iterator i = m_str.begin(), j = m_str.end();
    unsigned pos = 0;

    while (i != j && pos < startPos) {
        lookup->widthNext(i, j);
        ++pos;
    }

    return std::string(i, j);
}
