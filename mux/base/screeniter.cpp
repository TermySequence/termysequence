// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "screeniter.h"
#include "screen.h"
#include "buffer.h"

TermRowIterator::TermRowIterator(TermScreen *screen):
    m_screen(screen),
    done(false)
{
    // Initialization
    m_cur = m_end = m_screen->m_offset;
    int r = m_screen->m_buffer->size() - m_cur;
    int h = m_screen->m_bounds.height();
    m_end += (r > h) ? h : r;

    if (m_end > m_cur)
        y = 0;
    else
        done = true;
}

CellRow &
TermRowIterator::row()
{
    return m_screen->m_buffer->row(m_cur);
}

CellRow &
TermRowIterator::singleRow()
{
    return m_screen->m_buffer->singleRow(m_cur);
}

void
TermRowIterator::next()
{
    // Next row
    if (++m_cur < m_end)
        ++y;
    else
        done = true;
}
