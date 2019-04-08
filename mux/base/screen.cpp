// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "screen.h"
#include "emulator.h"
#include "region.h"
#include "regioniter.h"
#include "os/time.h"
#include "lib/attrstr.h"
#include "lib/unicode.h"
#include "config.h"

// Region states
#define STATE_NONE    0
#define STATE_PROMPT  1
#define STATE_COMMAND 2
#define STATE_OUTPUT  3

TermScreen::TermScreen(TermEmulator *emulator, const Size &size):
    m_buffer(emulator->buffer(0)),
    m_emulator(emulator),
    m_lookup(emulator->unicoding()),
    m_offset(0),
    m_row(&m_buffer->rawRow(0)),
    m_bounds(Point(), size),
    m_margins(Point(), size),
    m_stayWithinMargins(false),
    m_jobState(STATE_NONE)
{
}

TermScreen::TermScreen(TermEmulator *emulator, const TermScreen *copyfrom):
    m_buffer(emulator->buffer(0)),
    m_emulator(emulator),
    m_lookup(emulator->unicoding()),
    m_offset(copyfrom->m_offset),
    m_cursor(copyfrom->m_cursor),
    m_row(&m_buffer->rawRow(m_offset + m_cursor.y())),
    m_bounds(copyfrom->m_bounds),
    m_margins(copyfrom->m_margins),
    m_mousePos(copyfrom->m_mousePos),
    m_origin(copyfrom->m_origin),
    m_stayWithinMargins(copyfrom->m_stayWithinMargins),
    m_jobState(STATE_NONE)
{
}

TermScreen::~TermScreen()
{
    if (m_child)
        m_child->putReference();
    if (m_job)
        m_job->putReference();
}

bool
TermScreen::constrainToMargins()
{
    int &rx = m_cursor.rx();
    int &ry = m_cursor.ry();
    bool changed = false;

    if (rx < m_margins.left()) {
        rx = m_margins.left();
        changed = true;
    } else if (rx > m_margins.right()) {
        rx = m_margins.right();
        changed = true;
    }

    if (ry < m_margins.top()) {
        ry = m_margins.top();
        changed = true;
    } else if (ry > m_margins.bottom()) {
        ry = m_margins.bottom();
        changed = true;
    }

    if (changed)
        rowAndCursorUpdate();

    return changed;
}

void
TermScreen::setWidth(int width)
{
    int left = m_margins.left();
    int right = m_bounds.right() - m_margins.right();

    m_bounds.setWidth(width);

    if (left + right + 1 < width) {
        m_margins.setRight(m_bounds.right() - right);
    } else {
        m_margins.setLeft(0);
        m_margins.setRight(m_bounds.right());
    }

    constrainToMargins();
}

void
TermScreen::setHeight(int height, int linesAdded)
{
    int top = m_margins.top();
    int bottom = m_bounds.bottom() - m_margins.bottom();

    // adjust cursor
    if (height > m_bounds.height()) {
        m_cursor.ry() += height - m_bounds.height() - linesAdded;
    }

    m_bounds.setHeight(height);

    if (top + bottom + 1 < height) {
        m_margins.setBottom(m_bounds.bottom() - bottom);
    } else {
        m_margins.setTop(0);
        m_margins.setBottom(m_bounds.bottom());
    }

    moveToEnd();

    if (!constrainToMargins())
        rowAndCursorUpdate();
}

void
TermScreen::setBuffer(TermBuffer *buffer)
{
    if (m_buffer != buffer) {
        m_buffer = buffer;
        moveToEnd();
        rowAndCursorUpdate();
        m_emulator->reportBufferSwitched();
    }
}

static inline int
calculateRightBound(const CellRow &row, int limit)
{
    return (row.flags & Tsq::DblWidthLine) ? (limit / 2) : limit;
}

void
TermScreen::setMargins(const Rect &margins)
{
    if (m_margins != margins) {
        m_margins = margins;
        m_emulator->reportSizeChanged();
    }
    if (m_stayWithinMargins) {
        m_origin = m_margins.topLeft();
        constrainToMargins();
    }
}

void
TermScreen::setStayWithinMargins(bool stayWithinMargins)
{
    if (m_stayWithinMargins != stayWithinMargins) {
        m_stayWithinMargins = stayWithinMargins;

        if (stayWithinMargins) {
            m_origin = m_margins.topLeft();
            constrainToMargins();
        } else
            m_origin = Point();
    }
}

void
TermScreen::insertRow()
{
    if (!m_margins.contains(m_cursor))
        return;

    m_buffer->deleteRowAndInsertAbove(m_offset + m_margins.bottom(), m_offset + m_cursor.y());
    rowAndCursorUpdate();
}

void
TermScreen::deleteRow()
{
    if (!m_margins.contains(m_cursor))
        return;

    m_buffer->deleteRowAndInsertBelow(m_offset + m_cursor.y(), m_offset + m_margins.bottom());
    rowAndCursorUpdate();
}

void
TermScreen::scrollUp()
{
    int top = m_margins.top();

    if (top == 0) {
        m_buffer->insertRow(m_offset + m_margins.height());
        moveToEnd();
    } else {
        m_buffer->deleteRowAndInsertBelow(m_offset + top, m_offset + m_margins.bottom());
    }

    rowAndCursorUpdate();
}

void
TermScreen::scrollDown()
{
    m_buffer->deleteRowAndInsertAbove(m_offset + m_margins.bottom(), m_offset + m_margins.top());
    rowAndCursorUpdate();
}

void
TermScreen::scrollToJob()
{
    if (m_margins.top() == 0 && m_margins.bottom() == m_bounds.bottom())
        while (m_offset < m_job->startRow)
            scrollUp();
}

bool
TermScreen::cursorAtRight(int threshold) const
{
    int d = calculateRightBound(constRow(), m_margins.right()) - m_cursor.x();
    return (d >= 0) && (d < threshold);
};

void
TermScreen::cursorMoveX(bool relative, int x, bool stayWithinMargins)
{
    int &rx = m_cursor.rx();
    const CellRow &row = constRow();
    int leftBound, rightBound;

    if ((stayWithinMargins && m_margins.contains(m_cursor)) || m_stayWithinMargins) {
        leftBound = m_margins.left();
        rightBound = calculateRightBound(row, m_margins.right());
    } else {
        leftBound = 0;
        rightBound = calculateRightBound(row, m_bounds.right());
    }

    if (relative)
        rx += x;
    else
        rx = x + m_origin.x();

    if (rx < leftBound)
        rx = leftBound;
    else if (rx > rightBound)
        rx = rightBound;

    cursorUpdate(row);
}

void
TermScreen::cursorMoveY(bool relative, int y, bool stayWithinMargins)
{
    int &ry = m_cursor.ry();
    int topBound, bottomBound;

    if ((stayWithinMargins && m_margins.contains(m_cursor)) || m_stayWithinMargins) {
        topBound = m_margins.top();
        bottomBound = m_margins.bottom();
    } else {
        topBound = 0;
        bottomBound = m_bounds.bottom();
    }

    if (relative)
        ry += y;
    else
        ry = y + m_origin.y();

    if (ry < topBound)
        ry = topBound;
    else if (ry > bottomBound)
        ry = bottomBound;

    const CellRow &row = rowUpdate();

    if (row.flags & Tsq::DblWidthLine) {
        int &rx = m_cursor.rx();
        int rightBound = m_margins.right() / 2;
        if (rx > rightBound)
            rx = rightBound;
    }

    cursorUpdate(row);
}

void
TermScreen::cursorMoveDown()
{
    int &ry = m_cursor.ry();
    int topBound, bottomBound;

    if (m_margins.contains(m_cursor) || m_stayWithinMargins) {
        topBound = m_margins.top();
        bottomBound = m_margins.bottom();
    } else {
        topBound = 0;
        bottomBound = m_bounds.bottom();
    }

    ++ry;

    if (ry < topBound)
        ry = topBound;
    else if (ry > bottomBound)
        ry = bottomBound;

    CellRow &row = rowUpdate();
    m_buffer->touchRow(&row, ry + m_offset);

    if (row.flags & Tsq::DblWidthLine) {
        int &rx = m_cursor.rx();
        int rightBound = m_margins.right() / 2;
        if (rx > rightBound)
            rx = rightBound;
    }

    cursorUpdate(row);
}

void
TermScreen::cursorAdvance(unsigned dx)
{
    bool moved = false;

    do {
        if (cursorAtRight(1)) {
            setCursorPastEnd(true);
            break;
        }
        cursorMoveX(true, 1, true);
        moved = true;
    } while (--dx);

    if (!moved)
        cursorUpdate();
}

void
TermScreen::combineCell(const CellAttributes &a, codepoint_t c)
{
    if (likely(m_cursor.subpos() < MAX_CLUSTER_SIZE))
    {
        CellRow &r = m_buffer->singleRow(m_cursor.y() + m_offset);
        // updates cursor
        r.combine(m_cursor, a, c);
        m_cursor.incSubpos();
    }
}

void
TermScreen::writeCell(const CellAttributes &a, codepoint_t c, int width)
{
    int x = m_cursor.x();
    size_t nextptr;

    if (m_nextRow) {
        m_nextRow->flags &= ~Tsq::Continuation;
        m_buffer->touchRow(m_nextRow, m_cursor.y() + m_offset + 1);
        m_nextRow = nullptr;
    }
    m_buffer->touchRow(m_row, m_cursor.y() + m_offset);

    if (m_row->columns() == x) {
        nextptr = m_row->append(a, c, width);
    }
    else if (m_row->columns() > x) {
        // This handles the cases:
        // replacing double with single (first pos),
        // replacing double with single (second pos),
        // replacing 2 singles with double,
        // replacing a double partially over another double and a single
        // replacing a double over a single and partially over a double
        // replacing a double partially over two doubles
        // may update cursor (if a double was split)
        nextptr = m_row->replace(m_cursor, a, c, width, m_lookup);
    }
    else {
        m_row->pad(x - m_row->columns());
        nextptr = m_row->append(a, c, width);
    }

    // Advance cursor
    Tsq::CursorFlags flags;
    m_cursor.rptr() = nextptr;

    if (cursorAtRight(width)) {
        flags = Tsq::CursorPastEnd;
    } else {
        m_cursor.rx() += width;
        m_cursor.incPos();
        const char *next = m_row->str().data();
        const char *end = next + m_row->str().size();
        next += nextptr;
        width = (next < end) ? m_lookup->widthAt(next, end) : 0;
        flags = 0;
    }

    if (width == 2)
        flags |= Tsq::OnDoubleLeft;

    m_cursor.setSubpos(flags);
}

void
TermScreen::insertCells(int count)
{
    if (m_margins.contains(m_cursor))
    {
        CellRow &r = row(m_cursor.y());
        int x = m_cursor.x();
        int m = m_margins.right();

        do {
            if (r.columns() > m)
                r.remove(m, m_lookup);
            if (r.columns() > x)
                r.insert(x, m_lookup);
        } while (--count);

        cursorUpdate(r);
    }
}

void
TermScreen::deleteCell()
{
    if (m_margins.contains(m_cursor))
    {
        CellRow &r = row(m_cursor.y());
        int x = m_cursor.x();
        int m = m_margins.right();

        if (r.columns() > x)
            r.remove(x, m_lookup);
        if (r.columns() > m)
            r.insert(m, m_lookup);

        cursorUpdate(r);
    }
}

void
TermScreen::setLineFlags(int y, Tsq::LineFlags flags)
{
    CellRow &r = row(y);

    if (r.flags != flags) {
        r.flags = flags;

        if (flags & Tsq::DblWidthLine) {
            int rightBound = m_margins.right() / 2;
            if (m_cursor.y() == y) {
                int &rx = m_cursor.rx();
                if (rx > rightBound)
                    rx = rightBound;
            }
            r.resize(rightBound, m_lookup);
        }

        if (m_cursor.y() == y)
            cursorUpdate(r);
    }
}

void
TermScreen::resetLine(int y)
{
    CellRow &r = m_buffer->row(m_offset + y);
    r.clear();

    if (m_cursor.y() == y)
        cursorUpdate(r);
}

void
TermScreen::resetSingleLine(int y)
{
    CellRow &r = m_buffer->singleRow(m_offset + y);
    r.clear();

    if (m_cursor.y() == y)
        cursorUpdate(r);
}

inline void
TermScreen::endJobRegions()
{
    if (m_jobState != STATE_NONE) {
        if (m_child && m_child->putReference()) {
            m_child->end(m_buffer, this);
        }
        if (m_job && m_job->putReference()) {
            m_job->attributes[Tsq::attr_REGION_EXITCODE] = "-2";
            m_job->end(m_buffer, this);
        }
    }
}

void
TermScreen::beginPromptRegion()
{
    endJobRegions();

    m_job = new Region(Tsq::RegionJob);
    m_job->takeReference();
    m_job->begin(m_buffer, this);
    m_child = new Region(Tsq::RegionPrompt, m_job->id);
    m_child->takeReference();
    m_child->begin(m_buffer, this);

    m_jobState = STATE_PROMPT;
}

void
TermScreen::beginCommandRegion()
{
    if (m_jobState != STATE_PROMPT || m_job->refcount == 1)
        return;

    if (m_child->putReference())  {
        m_child->end(m_buffer, this);

        m_job->flags |= Tsq::HasPrompt;
        m_buffer->reportRegion(m_job);
    }

    m_child = new Region(Tsq::RegionCommand, m_job->id);
    m_child->takeReference();
    m_child->begin(m_buffer, this);

    m_jobState = STATE_COMMAND;
}

void
TermScreen::beginOutputRegion(const std::string *vars)
{
    if (m_jobState != STATE_COMMAND || m_job->refcount == 1)
        return;

    if (m_child->putReference())  {
        m_child->end(m_buffer, this);

        RegionStringBuilder builder(m_child, m_buffer, MAX_COMMAND_LINES);
        std::string &result = builder.build();

        if (result.find_first_not_of(' ') == std::string::npos) {
            m_child->flags |= Tsq::EmptyCommand;
            m_job->flags |= Tsq::EmptyCommand;
        }

        m_job->flags |= Tsq::HasCommand;
        m_job->attributes[Tsq::attr_REGION_COMMAND] = std::move(result);
        m_job->attributes[Tsq::attr_REGION_STARTED] = std::to_string(osWalltime());
        m_job->attributes[Tsq::attr_REGION_PATH] = vars[SH_PATH];
        m_job->attributes[Tsq::attr_REGION_USER] = vars[SH_USER];
        m_job->attributes[Tsq::attr_REGION_HOST] = vars[SH_HOST];
        m_buffer->reportRegion(m_job);
    }

    m_child = new Region(Tsq::RegionOutput, m_job->id);
    m_child->takeReference();
    m_child->begin(m_buffer, this);

    m_jobState = STATE_OUTPUT;
}

void
TermScreen::endOutputRegion(int code)
{
    if (m_jobState != STATE_OUTPUT)
        return;

    if (m_child->putReference())  {
        m_child->end(m_buffer, this);
        m_job->flags |= Tsq::HasOutput;
    }
    if (m_job->putReference()) {
        m_job->attributes[Tsq::attr_REGION_ENDED] = std::to_string(osWalltime());
        m_job->attributes[Tsq::attr_REGION_EXITCODE] = std::to_string(code);
        m_job->end(m_buffer, this);
    }

    m_job = nullptr;
    m_child = nullptr;
    m_jobState = STATE_NONE;
}

void
TermScreen::handlePartialCommand()
{
    if (m_jobState == STATE_COMMAND && m_child->refcount > 1)
    {
        RegionStringBuilder builder(m_child, m_buffer, 1);
        std::string &result = builder.build();
        m_emulator->setAttribute(Tsq::attr_COMMAND, std::move(result));
    }
}

void
TermScreen::reset() {
    endJobRegions();
    m_job = nullptr;
    m_child = nullptr;
    m_jobState = STATE_NONE;

    setMargins(m_bounds);
}
