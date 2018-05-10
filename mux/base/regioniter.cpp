// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "regioniter.h"
#include "buffer.h"
#include "emulator.h"
#include "region.h"
#include "lib/unicode.h"

RegionStringBuilder::RegionStringBuilder(const Region *region, const TermBuffer *buffer,
                                         unsigned maxLines):
    m_buffer(buffer),
    m_lookup(buffer->emulator()->unicoding()),
    m_region(region),
    m_maxLines(maxLines)
{
    m_size = m_buffer->size();
    m_cur = region->startRow;
    m_end = region->endRow;

    if (m_cur >= m_size) {
        m_done = true;
        return;
    }

    const CellRow &row = m_buffer->constRow(m_cur);

    if (m_cur == m_end) {
        m_result.append(row.substr(region->startCol, region->endCol, m_lookup));
        m_done = true;
    }
    else {
        m_result.append(row.substr(region->startCol, m_lookup));
    }
}

std::string &
RegionStringBuilder::build()
{
    if (!m_done)
        while (++m_cur < m_size)
        {
            const CellRow &row = m_buffer->constRow(m_cur);

            if (!(row.flags & Tsq::Continuation)) {
                if (--m_maxLines == 0)
                    break;

                m_result.push_back('\n');
            }

            if (m_cur == m_end) {
                m_result.append(row.substr(0, m_region->endCol, m_lookup));
                break;
            }
            else {
                m_result.append(row.str());
            }
        }

    while (!m_result.empty() && m_result.back() == '\n')
        m_result.pop_back();

    return m_result;
}
