// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "regioniter.h"
#include "region.h"
#include "bufferbase.h"
#include "term.h"
#include "lib/grapheme.h"

void
RegionStringBuilder::append(const CellRow &row, unsigned start, unsigned end)
{
    if (!(row.flags & Tsqt::NoSelect)) {
        Tsq::GraphemeWalk tbf(m_buffer->term()->unicoding(), row.str);
        column_t i = 0;

        for (; i < start && tbf.next(); ++i);
        size_t startptr = tbf.end();
        for (; i < end && tbf.next(); ++i);

        m_result.append(row.str.substr(startptr, tbf.end() - startptr));
    }
}

RegionStringBuilder::RegionStringBuilder(const BufferBase *buffer, const RegionBase *region):
    m_buffer(buffer),
    m_region(region),
    m_done(false)
{
    // NOTE this is where column_t collides with int for row sizing

    m_size = m_buffer->size();
    m_cur = region->startRow - m_buffer->origin();
    m_end = region->endRow - m_buffer->origin();

    if (m_cur >= m_size) {
        m_done = true;
        return;
    }

    const CellRow &row = m_buffer->row(m_cur);

    if (m_cur == m_end) {
        append(row, region->startCol, region->endCol);
        m_done = true;
    }
    else {
        append(row, region->startCol, row.size);
    }
}

QString
RegionStringBuilder::build(bool oneline)
{
    if (!m_done)
        while (++m_cur < m_size)
        {
            const CellRow &row = m_buffer->row(m_cur);

            if (!(row.flags & Tsq::Continuation)) {
                if (oneline)
                    break;
                m_result.push_back('\n');
            }

            if (m_cur == m_end) {
                append(row, 0, m_region->endCol);
                break;
            }
            else if (!(row.flags & Tsqt::NoSelect)) {
                m_result.append(row.str);
            }
        }

    return QString::fromStdString(m_result);
}
