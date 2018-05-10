// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/flags.h"
#include "bufferbase.h"
#include "region.h"

#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <vector>

class TermBuffers;

class TermBuffer final: public BufferBase
{
    friend class TermBuffers;

private:
    TermBuffers *m_parent;
    // controlled by TermBuffers
    size_t m_lower = 0;
    size_t m_upper = 0;

    // bounds of current update block
    index_t m_updatelo = 0;
    index_t m_updatehi = 0;

    index_t m_size = 0;
    index_t m_origin = 0;
    index_t m_capacity = 0;
    index_t m_capmask = 0;
    uint8_t m_caporder = 0;
    bool m_noScrollback = false;
    uint8_t m_bufid;

    std::deque<CellRow> m_rows;

    std::map<regionid_t,Region*> m_regions;
    std::map<regionid_t,Region*> m_semantics;
    std::unordered_set<const Region*> m_activeRegions;
    std::set<RegionRef,RegionStartSorter> m_regionsByStart;
    std::set<RegionRef,RegionEndSorter> m_regionsByEnd;

    // used by endUpdate
    std::string m_str;
    std::vector<unsigned> m_breaks;

private:
    CellRow& row(size_t i);

    Tsqt::CellFlags regionFlags(index_t i, column_t pos) const;
    void updateRow(CellRow &row, index_t i);
    bool searchRow(CellRow &row);

    void deleteRegion(Region *region, bool update);
    void deleteSemanticRegions(Region *region);
    void handleJobRegion(Region *region);
    void handleOutputRegion(Region *region, bool isNew);
    void handleUserRegion(Region *region);
    void handleImageRegion(Region *region, bool isNew);
    void handleContentRegion(Region *region);

    Region* safeRegion_(regionid_t id) const;
    Region* safeSemantic_(regionid_t id) const;
    const Region* findOutputRegion(index_t row) const;

    column_t xByPos(const CellRow &row, column_t pos) const;
    column_t xByPtr(index_t idx, size_t ptr) const;

public:
    TermBuffer(TermBuffers *parent, uint8_t bufid);
    TermBuffer(); // do not use this
    ~TermBuffer();

    inline TermBuffers* buffers() { return m_parent; }
    inline size_t size() const { return m_rows.size(); }
    inline size_t offset() const { return m_lower; }
    inline index_t origin() const { return m_origin; }
    inline uint8_t caporder() const { return m_caporder; }
    inline bool noScrollback() const { return m_noScrollback; }

    inline const CellRow& row(size_t i) const { return m_rows[(m_origin + i) & m_capmask]; }
    void setRow(index_t i, const uint32_t *data, const char *str, unsigned len, bool pushed);
    void setRegion(const uint32_t *data, AttributeMap &attributes);

    void updateRows(size_t start, size_t end, RegionList *regionret);
    index_t searchRows(size_t start, size_t end);
    void fetchRows(size_t start, size_t end);

    Cell cellByPos(size_t idx, column_t pos) const;
    Cell cellByX(size_t idx, int x) const;
    column_t posByX(size_t idx, int x) const;
    column_t xByPos(size_t idx, column_t pos) const;
    column_t xSize(size_t idx) const;
    column_t xByJavascript(index_t idx, size_t jspos) const;

    bool changeCapacity(index_t size, uint8_t capspec);
    void changeLength(index_t size);

    const Region* safeRegion(regionid_t id) const;
    const Region* firstRegion(Tsqt::RegionType type) const;
    const Region* lastRegion(Tsqt::RegionType type) const;
    const Region* nextRegion(Tsqt::RegionType type, regionid_t id) const;
    const Region* prevRegion(Tsqt::RegionType type, regionid_t id) const;
    const Region* downRegion(Tsqt::RegionType type, index_t row) const;
    const Region* upRegion(Tsqt::RegionType type, index_t row) const;

    const Region* modtimeRegion() const;
    const Region* findRegionByRow(Tsqt::RegionType type, index_t row) const;
    const Region* findRegionByParent(Tsqt::RegionType type, regionid_t id) const;

    const Region* safeSemantic(regionid_t id) const;
    void deleteSemantic(regionid_t);

    int recentJobs(const Region **buf, int n) const;
    bool endUpdate();
    void insertSemanticRegion(Region *region);

    inline const auto& activeRegions() const
    { return m_activeRegions; }
    inline void activateRegion(const Region *region)
    { m_activeRegions.insert(region); }
    inline void deactivateRegion(const Region *region)
    { m_activeRegions.erase(region); }
};

inline const Region *
TermBuffer::safeRegion(regionid_t id) const
{
    auto i = m_regions.find(id);
    return i != m_regions.end() ? i->second : nullptr;
}

inline const Region *
TermBuffer::safeSemantic(regionid_t id) const
{
    auto i = m_semantics.find(id);
    return i != m_semantics.end() ? i->second : nullptr;
}
