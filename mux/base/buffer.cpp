// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "buffer.h"
#include "emulator.h"

#include <unordered_set>

TermBuffer::TermBuffer(TermEmulator *emulator, unsigned screenHeight,
                       uint8_t caporder, uint8_t id) :
    m_emulator(emulator),
    m_size(screenHeight),
    m_realsize(screenHeight),
    m_modTimePtr(emulator->modTimePtr()),
    m_screenHeight(screenHeight),
    m_id(id),
    m_noScrollback(caporder == 0 ? 0x80 : 0)
{
    while ((1 << caporder) < screenHeight)
        ++caporder;

    m_capacity = 1 << caporder;
    m_capmask = m_capacity - 1;
    m_caporder = caporder;

    while (screenHeight--)
        m_rows.emplace_back();
}

TermBuffer::TermBuffer(TermEmulator *emulator, const TermBuffer *copyfrom) :
    m_emulator(emulator),
    m_size(copyfrom->m_size),
    m_realsize(copyfrom->m_realsize),
    m_capacity(copyfrom->m_capacity),
    m_capmask(copyfrom->m_capmask),
    m_modTimePtr(emulator->modTimePtr()),
    m_screenHeight(copyfrom->m_screenHeight),
    m_id(copyfrom->m_id),
    m_caporder(copyfrom->m_caporder),
    m_noScrollback(copyfrom->m_noScrollback),
    m_nextRegionId(copyfrom->m_nextRegionId),
    m_rows(copyfrom->m_rows),
    m_regions(copyfrom->m_regions)
{
    std::map<Region*,Region*> copies;

    for (auto &&i: m_regions) {
        Region *region = new Region(*i.second);
        region->refcount = 1;
        copies.emplace(i.second, region);
        i.second = region;
    }
    for (const auto &i: copyfrom->m_regionsByStart)
        m_regionsByStart.emplace(copies[i.ptr]);
    for (const auto &i: copyfrom->m_regionsByEnd)
        m_regionsByEnd.emplace(copies[i.ptr]);
}

TermBuffer::~TermBuffer()
{
    for (const auto &i: m_regions)
        i.second->putReference();
}

void
TermBuffer::deleteRegion(Region *region)
{
    if (region->type == Tsq::RegionImage)
        m_emulator->putContent(region);

    m_changedRegions.erase(region->bufreg());
    m_regions.erase(region->id);
    m_regionsByStart.erase(region);
    m_regionsByEnd.erase(region);

    region->putReference();
}

void
TermBuffer::insertRow(index_t pos)
{
    if (m_noScrollback) {
        m_rows.emplace(m_rows.begin() + pos);
        m_rows.pop_front();

        while (pos--)
            m_changedRows.emplace(pos);

        return;
    }
    else if (m_size < m_realsize)
    {
        index_t i = m_size;
        CellRow save = std::move(m_rows[i & m_capmask]);

        while (i > pos) {
            m_changedRows.emplace(i);
            m_rows[i & m_capmask] = std::move(m_rows[(i - 1) & m_capmask]);
            --i;
        }

        m_changedRows.emplace(i);
        m_rows[i & m_capmask] = std::move(save);

        goto out;
    }
    else if (m_size < m_capacity)
    {
        m_rows.emplace(m_rows.end() - (m_size - pos));

        while (pos <= m_size)
            m_changedRows.emplace_hint(m_changedRows.end(), pos++);

        goto out2;
    }
    else if (pos == m_size)
    {
        m_rows[pos & m_capmask].clear();
        m_changedRows.emplace_hint(m_changedRows.end(), pos);
    }
    else
    {
        index_t i = m_size;
        CellRow save = std::move(m_rows[i & m_capmask]);

        while (i > pos) {
            m_changedRows.emplace(i);
            m_rows[i & m_capmask] = std::move(m_rows[(i - 1) & m_capmask]);
            --i;
        }

        m_changedRows.emplace(i);
        m_rows[i & m_capmask] = std::move(save);
        m_rows[i & m_capmask].clear();
    }

    while (!m_regionsByStart.empty() &&
           (m_regionsByStart.begin()->ptr->startRow <= m_size - m_capacity))
        deleteRegion(m_regionsByStart.begin()->ptr);

out2:
    ++m_realsize;
out:
    ++m_size;
    m_emulator->reportBufferLength(m_id);

    if (m_changedRows.size() > m_screenHeight)
        m_changedRows.erase(m_changedRows.begin());
}

void
TermBuffer::deleteRowAndInsertAbove(index_t delpos, index_t addpos)
{
    index_t i = delpos;
    CellRow save = std::move(m_rows[i & m_capmask]);

    while (i > addpos) {
        m_changedRows.emplace(i);
        m_rows[i & m_capmask] = std::move(m_rows[(i - 1) & m_capmask]);
        --i;
    }

    m_changedRows.emplace(i);
    m_rows[i & m_capmask] = std::move(save);
    m_rows[i & m_capmask].clear();
}

void
TermBuffer::deleteRowAndInsertBelow(index_t delpos, index_t addpos)
{
    index_t i = delpos;
    CellRow save = std::move(m_rows[i & m_capmask]);

    while (i < addpos) {
        m_changedRows.emplace(i);
        m_rows[i & m_capmask] = std::move(m_rows[(i + 1) & m_capmask]);
        ++i;
    }

    m_changedRows.emplace(i);
    m_rows[i & m_capmask] = std::move(save);
    m_rows[i & m_capmask].clear();
}

void
TermBuffer::setCaporder(uint8_t caporder)
{
    if (m_caporder < caporder) {
        // Increase caporder
        index_t pos = m_realsize & m_capmask;

        if (pos < m_rows.size())
        {
            if (pos < m_capacity / 2) {
                // Move elements to back
                for (index_t i = 0; i < pos; ++i) {
                    CellRow save = std::move(m_rows.front());
                    m_rows.pop_front();
                    m_rows.emplace_back(std::move(save));
                }
            } else {
                // Move elements to front
                for (index_t i = pos; i < m_capacity; ++i) {
                    CellRow save = std::move(m_rows.back());
                    m_rows.pop_back();
                    m_rows.emplace_front(std::move(save));
                }
            }
        }

        // Remove saved rows
        while (m_realsize > m_size) {
            m_rows.pop_back();
            --m_realsize;
        }
    } else {
        // Decrease caporder
        // Find the two ends
        index_t capacity = 1 << caporder;
        index_t end = m_size & m_capmask;
        index_t start = (m_size > capacity) ? (end - capacity) & m_capmask : 0;

        if (end >= start) {
            // Remove elements from back
            m_rows.erase(m_rows.begin() + end, m_rows.end());
            // Remove elements from front
            m_rows.erase(m_rows.begin(), m_rows.begin() + start);
        } else {
            // Remove elements from middle
            m_rows.erase(m_rows.begin() + end, m_rows.begin() + start);
            // Move elements to back
            for (index_t i = 0; i < end; ++i) {
                CellRow save = std::move(m_rows.front());
                m_rows.pop_front();
                m_rows.emplace_back(std::move(save));
            }
        }
    }

    // Adjust regions
    m_size -= m_rows.size();
    m_changedRegions.clear();

    if (m_size) {
        while (!m_regionsByStart.empty() && m_regionsByStart.begin()->ptr->startRow < m_size)
            deleteRegion(m_regionsByStart.begin()->ptr);
        for (const auto &i: m_regionsByStart)
            i.ptr->startRow -= m_size;
        for (const auto &i: m_regionsByEnd)
            i.ptr->endRow -= m_size;
    }

    pullRegions(m_rows.size() - m_screenHeight, m_rows.size(), m_changedRegions);

    // Set new size and capacity
    m_size = m_realsize = m_rows.size();
    m_capacity = 1 << caporder;
    m_capmask = m_capacity - 1;
    m_caporder = caporder;

    m_changedRows.clear();
    for (unsigned i = 1; i <= m_screenHeight; ++i)
        m_changedRows.insert(m_size - i);
}

bool
TermBuffer::enableScrollback(uint8_t caporder)
{
    while ((1 << caporder) < m_screenHeight)
        ++caporder;

    if (m_noScrollback == 0 && m_caporder == caporder)
        return false;
    if (m_noScrollback != 0)
        m_noScrollback = 0;
    if (m_caporder != caporder)
        setCaporder(caporder);

    m_emulator->reportBufferCapacity(m_id);
    return true;
}

int
TermBuffer::setScreenHeight(unsigned screenHeight, unsigned maxChop)
{
    int added = 0, removed = 0;

    if (m_capacity < screenHeight) {
        uint8_t caporder = m_caporder;

        while ((1 << caporder) < screenHeight)
            ++caporder;

        setCaporder(caporder);
        m_emulator->reportBufferCapacity(m_id);
    }

    if (m_noScrollback) {
        while (m_size > screenHeight) {
            // size diverges from realsize here
            --m_size;
            ++removed;
            m_rows[m_size].clear();
        }
        while (m_size < screenHeight) {
            if (m_size == m_realsize) {
                m_rows.emplace_back();
                ++m_realsize;
            }
            m_changedRows.emplace_hint(m_changedRows.end(), m_size);
            ++m_size;
            ++added;
        }
    } else {
        while (m_size < screenHeight) {
            ++added;

            if (m_size < m_realsize)
                goto next;
            else if (m_size < m_capacity)
                m_rows.emplace_back();
            else
                m_rows[m_size & m_capmask].clear();

            ++m_realsize;
        next:
            m_changedRows.emplace_hint(m_changedRows.end(), m_size);
            ++m_size;
        }

        // try to chop empty rows instead of scrolling up
        for (unsigned i = screenHeight; maxChop && i < m_screenHeight; ++i) {
            CellRow &row = m_rows[(m_size - 1) & m_capmask];
            if (!row.isEmpty())
                break;

            // size diverges from realsize here
            row.clear();
            --m_size;
            ++removed;
            --maxChop;
        }
        for (unsigned i = screenHeight; i > m_screenHeight - added; --i) {
            m_changedRows.insert(m_size - i);
        }
    }

    m_screenHeight = screenHeight;
    if (added || removed)
        m_emulator->reportBufferLength(m_id);

    while (m_changedRows.size() > screenHeight)
        m_changedRows.erase(m_changedRows.begin());

    return added;
}

void
TermBuffer::clear()
{
    // Note: this should be called for alternate buffer only
    m_rows.clear();

    while (m_rows.size() < m_size) {
        m_changedRows.emplace_hint(m_changedRows.end(), m_rows.size());
        m_rows.emplace_back();
    }

    m_realsize = m_size;
}

bool
TermBuffer::clearScrollback()
{
    if (m_size == m_screenHeight)
        return false;

    // Find the two ends
    index_t end = m_size & m_capmask;
    index_t start = (end - m_screenHeight) & m_capmask;

    if (end >= start) {
        // Remove elements from back
        m_rows.erase(m_rows.begin() + end, m_rows.end());
        // Remove elements from front
        m_rows.erase(m_rows.begin(), m_rows.begin() + start);
    } else {
        // Remove elements from middle
        m_rows.erase(m_rows.begin() + end, m_rows.begin() + start);
        // Move elements to back
        for (index_t i = 0; i < end; ++i) {
            CellRow save = std::move(m_rows.front());
            m_rows.pop_front();
            m_rows.emplace_back(std::move(save));
        }
    }

    // Adjust regions
    m_size -= m_screenHeight;
    m_changedRegions.clear();

    if (m_size) {
        while (!m_regionsByStart.empty() && m_regionsByStart.begin()->ptr->startRow < m_size)
            deleteRegion(m_regionsByStart.begin()->ptr);
        for (const auto &i: m_regionsByStart)
            i.ptr->startRow -= m_size;
        for (const auto &i: m_regionsByEnd)
            i.ptr->endRow -= m_size;
    }

    pullRegions(0, m_screenHeight, m_changedRegions);

    // Set new size
    m_size = m_realsize = m_screenHeight;

    m_changedRows.clear();
    for (index_t i = 0; i < m_size; ++i)
        m_changedRows.insert(i);

    m_emulator->reportBufferCapacity(m_id);
    return true;
}

void
TermBuffer::reportRegion(Region *region)
{
    m_changedRegions.insert(region->id);
}

void
TermBuffer::removeRegions(index_t startRow, column_t startCol)
{
    // Delete any non-user regions starting at or after the given position
    for (auto i = m_regionsByStart.rbegin(), j = m_regionsByStart.rend(); i != j; ++i) {
        Region *other = i->ptr;

        if (other->startRow < startRow)
            break;
        if (other->type == Tsq::RegionUser)
            continue;
        if (other->startRow == startRow && other->startCol < startCol)
            continue;
        if (other->flags & Tsq::Deleted)
            continue;

        other->flags |= Tsq::Deleted;
        m_changedRegions.insert(other->bufreg());
    }
    // Overwrite any regions ending after the given position
    for (auto i = m_regionsByEnd.rbegin(), j = m_regionsByEnd.rend(); i != j; ++i) {
        Region *other = i->ptr;

        if (other->endRow < startRow)
            break;
        if (other->endRow == startRow && other->endCol <= startCol)
            continue;
        if (other->flags & Tsq::Deleted)
            continue;

        other->flags |= Tsq::Overwritten;
        m_changedRegions.insert(other->bufreg());
    }
}

void
TermBuffer::addRegion(Region *region)
{
    ++m_nextRegionId;
    m_nextRegionId += (m_nextRegionId == INVALID_REGION_ID);

    region->id = m_nextRegionId;
    region->bufid = m_id;

    m_regions.emplace_hint(m_regions.end(), region->id, region);
    m_regionsByStart.emplace_hint(m_regionsByStart.end(), region);
    // Note: not added to m_regionsByEnd here
    m_changedRegions.emplace_hint(m_changedRegions.end(), region->bufreg());
}

void
TermBuffer::beginRegion(Region *region)
{
    // Delete any same-type regions starting at or after this one
    for (auto i = m_regionsByStart.rbegin(), j = m_regionsByStart.rend(); i != j; ++i) {
        Region *other = i->ptr;

        if (other->startRow < region->startRow)
            break;
        if (other->type != region->type)
            continue;
        if (other->startRow == region->startRow && other->startCol < region->startCol)
            continue;
        if (other->flags & Tsq::Deleted)
            continue;

        other->flags |= Tsq::Deleted;
        m_changedRegions.insert(other->id);
    }
    // Overwrite any regions ending after this one
    for (auto i = m_regionsByEnd.rbegin(), j = m_regionsByEnd.rend(); i != j; ++i) {
        Region *other = i->ptr;

        if (other->endRow < region->startRow)
            break;
        if (other->endRow == region->startRow && other->endCol <= region->startCol)
            continue;
        if (other->flags & Tsq::Deleted)
            continue;

        other->flags |= Tsq::Overwritten;
        m_changedRegions.insert(other->id);
    }

    addRegion(region);
}

void
TermBuffer::endRegion(Region *region)
{
    m_regionsByEnd.emplace_hint(m_regionsByEnd.end(), region);
    m_changedRegions.insert(region->id);
}

void
TermBuffer::pullRegions(index_t start, index_t end, std::set<bufreg_t> &ret) const
{
    Region lower(Tsq::RegionLowerBound);
    lower.startRow = lower.endRow = start;
    Region upper(Tsq::RegionUpperBound);
    upper.startRow = upper.endRow = end - 1;

    auto i = m_regionsByStart.lower_bound(&lower);
    auto j = m_regionsByStart.upper_bound(&upper);
    auto k = m_regionsByEnd.lower_bound(&lower);
    auto l = m_regionsByEnd.upper_bound(&upper);

    for (; i != j; ++i) {
        ret.insert(i->ptr->pbufreg());
        ret.insert(i->ptr->bufreg());
    }
    for (; k != l; ++k) {
        ret.insert(k->ptr->pbufreg());
        ret.insert(k->ptr->bufreg());
    }

    ret.erase(MAKE_BUFREG(m_id, INVALID_REGION_ID));
}

void
TermBuffer::pullRegions(index_t start, index_t end, std::vector<Region> &ret) const
{
    Region lower(Tsq::RegionLowerBound);
    lower.startRow = lower.endRow = start;
    Region upper(Tsq::RegionUpperBound);
    upper.startRow = upper.endRow = end - 1;

    auto i = m_regionsByStart.lower_bound(&lower);
    auto j = m_regionsByStart.upper_bound(&upper);
    auto k = m_regionsByEnd.lower_bound(&lower);
    auto l = m_regionsByEnd.upper_bound(&upper);

    std::map<regionid_t,const Region*> map;

    for (auto m = i; m != j; ++m)
        map.emplace(m->ptr->id, m->ptr);
    for (auto m = k; m != l; ++m)
        map.emplace(m->ptr->id, m->ptr);
    for (; i != j; ++i)
        map.emplace(i->ptr->parent, nullptr);
    for (; k != l; ++k)
        map.emplace(k->ptr->parent, nullptr);

    map.erase(INVALID_REGION_ID);
    const Region *p;

    for (const auto &ent: map)
        if ((p = ent.second) || (p = safeRegion(ent.first)))
            ret.emplace_back(*p);
}

static inline bool
checkRegion(Region *region)
{
    return (region->startRow < region->endRow) ||
        (region->startRow == region->endRow && region->startCol <= region->endCol);
}

regionid_t
TermBuffer::addUserRegion(Region *region)
{
    if (!checkRegion(region) || region->type != Tsq::RegionUser)
        return INVALID_REGION_ID;

    // Find existing overlapping regions
    Region lower(Tsq::RegionLowerBound);
    lower.startRow = lower.endRow = region->startRow;
    Region upper(Tsq::RegionUpperBound);
    upper.startRow = upper.endRow = region->endRow;

    auto i = m_regionsByStart.lower_bound(&lower);
    auto j = m_regionsByStart.upper_bound(&upper);
    auto k = m_regionsByEnd.lower_bound(&lower);
    auto l = m_regionsByEnd.upper_bound(&upper);

    std::unordered_set<Region*> deleteRegions;

    while (i != j) {
        if (i->ptr->type == Tsq::RegionUser && region->overlaps(i->ptr)) {
            deleteRegions.insert(i->ptr);
        }
        ++i;
    }
    while (k != l) {
        if (k->ptr->type == Tsq::RegionUser && region->overlaps(k->ptr)) {
            deleteRegions.insert(k->ptr);
        }
        ++k;
    }

    for (auto i: deleteRegions)
        deleteRegion(i);

    ++m_nextRegionId;
    m_nextRegionId += (m_nextRegionId == INVALID_REGION_ID);

    region->id = m_nextRegionId;
    region->bufid = m_id;
    region->flags = (Tsq::HasStart|Tsq::HasEnd);

    m_regions.emplace(region->id, region);
    m_regionsByStart.emplace(region);
    m_regionsByEnd.emplace(region);
    return region->id;
}

bool
TermBuffer::removeUserRegion(regionid_t id)
{
    auto i = m_regions.find(id);

    if (i != m_regions.end())
    {
        Region *region = i->second;
        if (region->type == Tsq::RegionUser && !(region->flags & Tsq::Deleted))
        {
            region->flags |= Tsq::Deleted;
            return true;
        }
    }
    return false;
}
