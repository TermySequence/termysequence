// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "rect.h"
#include "lib/flags.h"

class CursorBase: public Point
{
protected:
    Tsq::CursorFlags m_flags;
    int32_t m_pos;

public:
    inline CursorBase() : m_flags(0), m_pos(0) {}
    inline CursorBase(int x) : Point(x, 0), m_flags(0), m_pos(0) {}

    inline unsigned flags() const { return m_flags; }
    inline bool pastEnd() const { return m_flags & Tsq::CursorPastEnd; }
    inline int32_t pos() const { return m_pos; }

    inline bool operator==(const CursorBase &c) const {
        return Point::operator==(c) && m_flags == c.m_flags && m_pos == c.m_pos;
    }
    inline bool operator!=(const CursorBase &c) const {
        return Point::operator!=(c) || m_flags != c.m_flags || m_pos != c.m_pos;
    }
};

class Cursor final: public CursorBase
{
private:
    size_t m_ptr;
    int32_t m_savedPos;

public:
    inline Cursor() : m_ptr(0) {}
    inline Cursor(int x) : CursorBase(x), m_ptr(0) {}

    inline size_t ptr() const { return m_ptr; }
    inline int32_t savedPos() const { return m_savedPos; }

    inline size_t& rptr() { return m_ptr; }
    inline int32_t& rpos() { return m_pos; }
    inline void incPos() { m_savedPos = m_pos++; }

    inline uint8_t subpos() const { return m_flags & 0xff; }
    inline void incSubpos() { ++m_flags; }
    inline void setSubpos(Tsq::CursorFlags addFlags) { m_flags = addFlags|1; }
    inline void reset() { m_flags = 0; }

    inline void setDoubleLeft() { m_flags |= Tsq::OnDoubleLeft; }
    inline void setDoubleRight() { m_flags |= Tsq::OnDoubleRight; }

    inline void setPastEnd(bool pastEnd) {
        if (pastEnd)
            m_flags |= Tsq::CursorPastEnd;
        else
            m_flags &= ~Tsq::CursorPastEnd;
    }
};
