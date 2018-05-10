// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "thumbport.h"
#include "term.h"
#include "buffers.h"
#include "screen.h"

TermThumbport::TermThumbport(TermInstance *term, QObject *parent):
    TermViewport(term, term->screen()->size(), parent),
    m_screen(term->screen())
{
    m_offset = m_screen->offset();

    connect(term, SIGNAL(sizeChanged(QSize)), SLOT(handleSizeChanged(QSize)));

    connect(m_screen, SIGNAL(offsetChanged(int)), SLOT(handleScreenMoved(int)));
    connect(m_screen, SIGNAL(inputReceived()), this, SIGNAL(inputReceived()));
}

inline void
TermThumbport::doUpdateRegions()
{
    m_buffers->updateRows(m_offset, m_offset + m_bounds.height(), &m_regions);
}

void
TermThumbport::handleScreenMoved(int offset)
{
    m_offset = offset;
    doUpdateRegions();
}

void
TermThumbport::handleSizeChanged(QSize size)
{
    m_bounds = size;
    doUpdateRegions();
}

QPoint
TermThumbport::cursor() const
{
    QPoint cursor = m_screen->cursor();
    cursor.setY(cursor.y() + (int)(m_screen->offset() - m_offset));
    return cursor;
}

void
TermThumbport::updateRegions()
{
    doUpdateRegions();
}
