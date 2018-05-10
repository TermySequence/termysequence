// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "screen.h"
#include "term.h"
#include "buffers.h"

TermScreen::TermScreen(TermInstance *term, const QSize &size, QObject *parent):
    TermViewport(term, size, parent)
{
    connect(m_buffers, SIGNAL(bufferChanged()), SLOT(handleBufferChanged()));
    connect(term, SIGNAL(sizeChanged(QSize)), SLOT(handleSizeChanged(QSize)));

    moveToEnd();
}

void
TermScreen::handleBufferChanged()
{
    moveToEnd();
}

void
TermScreen::handleSizeChanged(QSize size)
{
    m_bounds = size;
    moveToEnd();
}

void
TermScreen::setCursor(QPoint cursor)
{
    m_cursor = cursor;

    int row = cursor.y() + m_offset;

    if (m_row != row)
        emit cursorMoved(m_row = row);
}
