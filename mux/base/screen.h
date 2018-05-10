// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "cell.h"
#include "buffer.h"
#include "cursor.h"

namespace Tsq { class Unicoding; }
class TermEmulator;
class Region;

// beginOutputRegion var indices
#define SH_PATH 0
#define SH_USER 1
#define SH_HOST 2

class TermScreen
{
    friend class TermRowIterator;

private:
    TermBuffer *m_buffer;
    TermEmulator *m_emulator;
    Tsq::Unicoding *m_lookup;
    index_t m_offset;

    Cursor m_cursor;
    CellRow *m_row, *m_nextRow = nullptr;

    Rect m_bounds;
    Rect m_margins;
    Point m_mousePos;
    Point m_origin;
    bool m_stayWithinMargins;

    int m_jobState;
    Region *m_job = nullptr;
    Region *m_child = nullptr;

    bool constrainToMargins();
    bool cursorAtRight(int threshold) const;

public:
    TermScreen(TermEmulator *emulator, const Size &size);
    TermScreen(TermEmulator *emulator, const TermScreen *copyfrom);
    ~TermScreen();

    inline TermBuffer* buffer() { return m_buffer; }
    inline index_t offset() const { return m_offset; }
    inline int width() const { return m_bounds.width(); }
    inline int height() const { return m_bounds.height(); }
    inline const Size& size() const { return m_bounds; }
    inline const Rect& bounds() const { return m_bounds; }

    inline const Rect& margins() const { return m_margins; }
    inline const Cursor& cursor() const { return m_cursor; }
    inline const Point& mousePos() const { return m_mousePos; }
    inline bool stayWithinMargins() const { return m_stayWithinMargins; }

    void setBuffer(TermBuffer *buffer);
    void setWidth(int width);
    void setHeight(int height, int linesAdded);
    void setMargins(const Rect &margins);
    void setStayWithinMargins(bool stayWithinMargins);
    inline void setCursorPastEnd(bool cursorPastEnd) { m_cursor.setPastEnd(cursorPastEnd); }
    inline void setMousePos(const Point &mousePos) { m_mousePos = mousePos; }

    inline bool cursorAtLeft() const { return m_cursor.x() == m_margins.left(); }
    inline bool cursorAtTop() const { return m_cursor.y() == m_margins.top(); }
    inline bool cursorAtBottom() const { return m_cursor.y() == m_margins.bottom(); }
    inline bool cursorPastEnd(int width) const;

    inline void cursorUpdate(const CellRow &row);
    inline void cursorUpdate();
    inline CellRow& rowUpdate();
    inline void rowAndCursorUpdate();

    void cursorAdvance(unsigned dx);
    void cursorMoveX(bool relative, int x, bool stayWithinMargins);
    void cursorMoveY(bool relative, int y, bool stayWithinMargins);
    void cursorMoveDown();
    void scrollUp();
    void scrollDown();
    void scrollToJob();
    inline void moveToEnd() { m_offset = m_buffer->size() - m_bounds.height(); }

    inline const CellRow& constRow(int y) const { return m_buffer->constRow(m_offset + y); }
    inline const CellRow& constRow() const { return constRow(m_cursor.y()); }
    inline CellRow& row(int y) { return m_buffer->row(m_offset + y); }
    inline CellRow& row() { return row(m_cursor.y()); }
    void insertRow();
    void deleteRow();

    void writeCell(const CellAttributes &a, codepoint_t c, int width);
    void combineCell(const CellAttributes &a, codepoint_t c);
    void insertCells(int count);
    void deleteCell();

    void setLineFlags(int y, Tsq::LineFlags flags);
    inline void setLineFlags(Tsq::LineFlags flags) { setLineFlags(m_cursor.y(), flags); }
    void resetLine(int y);
    void resetSingleLine(int y);
    void reset();

    void beginPromptRegion();
    void beginCommandRegion();
    void beginOutputRegion(const std::string *vars);
    void endOutputRegion(int code);
    void endJobRegions();

    void handlePartialCommand();
};

inline bool
TermScreen::cursorPastEnd(int width) const
{
    return m_cursor.pastEnd() || (width == 2 && cursorAtRight(1));
}
