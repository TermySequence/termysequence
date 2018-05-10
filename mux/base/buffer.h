// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "cell.h"
#include "region.h"

#include <deque>
#include <map>
#include <set>

class TermEmulator;

class TermBuffer
{
private:
    TermEmulator *m_emulator;
    index_t m_size;
    index_t m_realsize;
    index_t m_capacity;
    index_t m_capmask;
    const int32_t *m_modTimePtr;
    unsigned m_screenHeight;
    uint8_t m_id;
    uint8_t m_caporder;
    uint8_t m_noScrollback;
    regionid_t m_nextRegionId = 0;

    std::deque<CellRow> m_rows;
    std::set<index_t> m_changedRows;

    std::set<bufreg_t> m_changedRegions;
    std::map<regionid_t,Region*> m_regions;
    std::set<RegionRef,RegionStartSorter> m_regionsByStart;
    std::set<RegionRef,RegionEndSorter> m_regionsByEnd;

    void setCaporder(uint8_t caporder);
    void deleteRegion(Region *region);

public:
    TermBuffer(TermEmulator *emulator, unsigned screenHeight,
               uint8_t caporder, uint8_t id);
    TermBuffer(TermEmulator *emulator, const TermBuffer *copyfrom);
    ~TermBuffer();

    inline const TermEmulator* emulator() const { return m_emulator; }
    inline index_t size() const { return m_size; }
    inline index_t capacity() const { return m_capacity; }
    inline unsigned screenHeight() const { return m_screenHeight; }
    inline uint8_t id() const { return m_id; }
    inline uint8_t caporder() const { return m_caporder|m_noScrollback; }
    inline bool noScrollback() const { return m_noScrollback != 0; }

    inline const CellRow& constRow(index_t i) const;
    inline CellRow& rawRow(index_t i);
    inline CellRow& row(index_t i);
    inline CellRow& singleRow(index_t i);
    inline void touchRow(CellRow *row, index_t i);

    inline const auto& changedRows() const { return m_changedRows; }
    inline const auto& changedRegions() const { return m_changedRegions; }
    inline const Region* region(regionid_t id) const { return m_regions.at(id); }
    const Region* safeRegion(regionid_t id) const;
    void resetEventState();

    void insertRow(index_t pos);
    void deleteRowAndInsertAbove(index_t delpos, index_t addpos);
    void deleteRowAndInsertBelow(index_t delpos, index_t addpos);

    bool enableScrollback(uint8_t caporder);
    bool clearScrollback();
    int setScreenHeight(unsigned screenHeight, unsigned maxChop);
    void clear();

    void reportRegion(Region *region);
    void removeRegions(index_t startRow, column_t startCol);
    void addRegion(Region *region);
    void beginRegion(Region *region);
    void endRegion(Region *region);
    void pullRegions(index_t start, index_t end, std::set<bufreg_t> &ret) const;
    void pullRegions(index_t start, index_t end, std::vector<Region> &ret) const;

    regionid_t addUserRegion(Region *region);
    bool removeUserRegion(regionid_t region);
};

inline const CellRow &
TermBuffer::constRow(index_t i) const
{
    return m_rows[i & m_capmask];
}

inline CellRow &
TermBuffer::rawRow(index_t i)
{
    return m_rows[i & m_capmask];
}

inline CellRow &
TermBuffer::row(index_t i)
{
    m_changedRows.emplace_hint(m_changedRows.end(), i);

    // Clear continuation bit on following line
    if (i < m_size - 1) {
        CellRow &next = m_rows[(i + 1) & m_capmask];
        if (next.flags) {
            m_changedRows.emplace_hint(m_changedRows.end(), i + 1);
            next.modtime = *m_modTimePtr;
            next.flags &= ~Tsq::Continuation;
        }
    }

    CellRow &row = m_rows[i & m_capmask];
    row.modtime = *m_modTimePtr;
    return row;
}

inline CellRow &
TermBuffer::singleRow(index_t i)
{
    // No continuation check
    m_changedRows.emplace_hint(m_changedRows.end(), i);
    CellRow &row = m_rows[i & m_capmask];
    row.modtime = *m_modTimePtr;
    return row;
}

inline void
TermBuffer::touchRow(CellRow *row, index_t i)
{
    m_changedRows.emplace_hint(m_changedRows.end(), i);
    row->modtime = *m_modTimePtr;
}

inline const Region *
TermBuffer::safeRegion(regionid_t id) const
{
    auto i = m_regions.find(id);
    return i != m_regions.end() ? i->second : nullptr;
}

inline void
TermBuffer::resetEventState()
{
    m_changedRows.clear();
    m_changedRegions.clear();
}
