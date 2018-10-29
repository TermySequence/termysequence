// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "buffers.h"
#include "buffer.h"
#include "selection.h"
#include "term.h"
#include "overlay.h"

#define NBUFFERS 3

static const CellRow s_mtrow;

TermBuffers::TermBuffers(TermInstance *term) :
    QObject(term),
    BufferBase(term, new Selection(this))
{
    m_buffers = new TermBuffer[NBUFFERS] {
        { this, 0 },
        { this, 0 },
        { this, 1 }
    };

    // Set up separator buffers
    for (int b = 1; b < NBUFFERS; b += 2) {
        CellRow &separatorRow = m_buffers[b].m_rows.emplace_back();
        separatorRow.flags = Tsqt::Downloaded|Tsqt::NoSelect;
        for (int i = 0; i < 128; ++i)
            separatorRow.str.append("\xE2\x94\x81""\xE2\x94\x81", 6);

        Cell &cell = separatorRow.cells.emplace_back();
        cell.flags = Tsqt::PaintU2500;
        cell.endptr = separatorRow.str.size();
        cell.cellwidth = 256;
    }

    recalculateSizes();
}

TermBuffers::~TermBuffers()
{
    delete [] m_buffers;
    delete m_selection;
}

inline void
TermBuffers::recalculateSizes()
{
    m_size = m_buffers[0].m_upper = m_buffers[0].size();

    for (int b = 1; b <= m_activeBuffer; ++b) {
        m_buffers[b].m_lower = m_size;
        m_size += m_buffers[b].size();
        m_buffers[b].m_upper = m_size;
    }
}

TermBuffer *
TermBuffers::bufferAt(size_t idx)
{
    int b = 0;
    while (b < m_activeBuffer && idx >= m_buffers[b].m_upper)
        b += 2;

    return m_buffers + b;
}

void
TermBuffers::setActiveBuffer(uint8_t bufid)
{
    bufid *= 2;
    if (m_activeBuffer != bufid && bufid < NBUFFERS) {
        m_activeBuffer = bufid;
        recalculateSizes();

        if (m_selectionActive && m_selection->isAfter(m_buffers[bufid].m_upper)) {
            m_selection->clear();
        }

        // emit bufferSwitched(bufid);
        emit bufferChanged();
    }
}

void
TermBuffers::beginSetOverlay(const TermOverlay *overlay)
{
    if (m_selectionActive)
        m_selection->clear();

    m_overlay = overlay;
}

void
TermBuffers::endSetOverlay()
{
    emit bufferChanged();
    emit contentChanged();
}

void
TermBuffers::changeCapacity(uint8_t bufid, index_t rows, uint8_t capspec)
{
    bufid *= 2;
    if (bufid < NBUFFERS) {
        bool rc = m_buffers[bufid].changeCapacity(rows, capspec);
        recalculateSizes();

        if (rc) {
            emit bufferReset();
        }
        emit bufferChanged();
    }
}

void
TermBuffers::changeLength(uint8_t bufid, index_t rows)
{
    bufid *= 2;
    if (bufid < NBUFFERS) {
        m_buffers[bufid].changeLength(rows);
        recalculateSizes();

        emit bufferChanged();
    }
}

const CellRow &
TermBuffers::row(size_t i) const
{
    int b;

    if (m_overlay)
        return m_overlay->row(i);

    for (b = 0; b < NBUFFERS - 1; ++b)
        if (i < m_buffers[b].m_upper)
            break;

    const auto *ptr = m_buffers + b;
    return ptr->row(i - ptr->m_lower);
}

const CellRow &
TermBuffers::safeRow(size_t i) const
{
    return i < size() ? row(i) : s_mtrow;
}

void
TermBuffers::setRow(index_t i, const uint32_t *data, const char *str, unsigned len, bool pushed)
{
    uint8_t bufid = (data[0] & 0xff) * 2;
    if (bufid < NBUFFERS) {
        m_buffers[bufid].setRow(i, data, str, len, pushed);
    }

    if (!pushed) {
        if (i == m_fetchNext)
            emit fetchFetchRow();
        if (m_term->searching())
            emit fetchScanRow(i);
    }
}

void
TermBuffers::setRegion(const uint32_t *data, AttributeMap &attributes)
{
    uint8_t bufid = (data[1] & 0xff) * 2;
    if (bufid < NBUFFERS) {
        m_buffers[bufid].setRegion(data, attributes);
        m_regionChanged = true;
    }
}

void
TermBuffers::updateRows(size_t start, size_t end, RegionList *regionret)
{
    if (regionret) {
        regionret->list.clear();
        if (m_overlay)
            regionret = nullptr;
    }

    if (end > m_size)
        end = m_size;

    for (int b = 0; start < end; ++b) {
        if (start < m_buffers[b].m_upper) {
            size_t bound = (end < m_buffers[b].m_upper) ? end : m_buffers[b].m_upper;
            m_buffers[b].updateRows(start - m_buffers[b].m_lower,
                                    bound - m_buffers[b].m_lower, regionret);
            start = bound;
        }
    }
}

index_t
TermBuffers::searchRows(size_t start, size_t end)
{
    index_t result = INVALID_INDEX;

    if (end > m_size)
        end = m_size;

    for (int b = 0; start < end; ++b) {
        if (start < m_buffers[b].m_upper) {
            size_t bound = (end < m_buffers[b].m_upper) ? end : m_buffers[b].m_upper;
            index_t rc = m_buffers[b].searchRows(start - m_buffers[b].m_lower,
                                                 bound - m_buffers[b].m_lower);
            if (rc != INVALID_INDEX)
                result = rc;

            start = bound;
        }
    }

    return result;
}

void
TermBuffers::fetchRows(size_t start, size_t end)
{
    if (end > m_size)
        end = m_size;

    for (int b = 0; start < end; ++b) {
        if (start < m_buffers[b].m_upper) {
            size_t bound = (end < m_buffers[b].m_upper) ? end : m_buffers[b].m_upper;
            m_buffers[b].fetchRows(start - m_buffers[b].m_lower, bound - m_buffers[b].m_lower);
            start = bound;
        }
    }
}

Cell
TermBuffers::cellByPos(size_t i, column_t pos) const
{
    if (!m_overlay)
        for (int b = 0; b < NBUFFERS; ++b)
            if (i < m_buffers[b].m_upper)
                return m_buffers[b].cellByPos(i - m_buffers[b].m_lower, pos);

    return Cell();
}

Cell
TermBuffers::cellByX(size_t i, int x) const
{
    if (!m_overlay)
        for (int b = 0; b < NBUFFERS; ++b)
            if (i < m_buffers[b].m_upper)
                return m_buffers[b].cellByX(i - m_buffers[b].m_lower, x);

    return Cell();
}

column_t
TermBuffers::posByX(size_t i, int x) const
{
    if (!m_overlay)
        for (int b = 0; b < NBUFFERS; ++b)
            if (i < m_buffers[b].m_upper)
                return m_buffers[b].posByX(i - m_buffers[b].m_lower, x);

    return 0;
}

column_t
TermBuffers::xByPos(index_t index, column_t pos) const
{
    size_t i = index - origin();

    if (!m_overlay)
        for (int b = 0; b < NBUFFERS; ++b)
            if (i < m_buffers[b].m_upper)
                return m_buffers[b].xByPos(i - m_buffers[b].m_lower, pos);

    return 0;
}

column_t
TermBuffers::xSize(index_t index) const
{
    size_t i = index - origin();

    if (!m_overlay)
        for (int b = 0; b < NBUFFERS; ++b)
            if (i < m_buffers[b].m_upper)
                return m_buffers[b].xSize(i - m_buffers[b].m_lower);

    return 0;
}

void
TermBuffers::beginUpdate()
{
    for (int b = 0; b < NBUFFERS; b += 2) {
        m_buffers[b].m_updatelo = INVALID_INDEX;
        m_buffers[b].m_updatehi = 0;
    }
}

void
TermBuffers::endUpdate()
{
    for (int b = 0; b < NBUFFERS; b += 2)
        if (m_buffers[b].endUpdate())
            m_regionChanged = true;

    if (m_regionChanged) {
        m_regionChanged = false;
        ++m_regionState;
        emit regionChanged();
    }

    emit contentChanged();
}

void
TermBuffers::activateRegion(const Region *region)
{
    m_buffers->activateRegion(region);
    reportRegionChanged();
}

void
TermBuffers::deactivateRegion(const Region *region)
{
    m_buffers->deactivateRegion(region);
    reportRegionChanged();
}

void
TermBuffers::activateSelection()
{
    if (!m_selectionActive) {
        m_selectionActive = true;

        for (int b = 0; b < NBUFFERS; ++b)
            m_buffers[b].setSelection(m_selection);
    }

    reportRegionChanged();
}

void
TermBuffers::deactivateSelection()
{
    if (m_selectionActive) {
        m_selectionActive = false;

        for (int b = 0; b < NBUFFERS; ++b)
            m_buffers[b].setSelection(nullptr);

        reportRegionChanged();
    }
}
