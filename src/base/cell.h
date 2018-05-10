// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/flags.h"
#include "lib/types.h"

#include <QString>
#include <QColor>
#include <QRectF>
#include <vector>
#include <initializer_list>

//
// Structure definitions
//
struct CellAttributes32
{
    Tsq::CellFlags flags;
    uint32_t fg;
    uint32_t bg;

    CellAttributes32();
    CellAttributes32(std::initializer_list<uint32_t> l);

    bool operator==(const CellAttributes32 &a) const;
    bool operator!=(const CellAttributes32 &a) const;
};

struct CellAttributes
{
    Tsqt::CellFlags flags;
    uint32_t fg;
    uint32_t bg;
    regionid_t link;

    CellAttributes();
    CellAttributes(const uint32_t *ptr);
    CellAttributes(const CellAttributes32 &a);

    bool isVisible(QChar val) const;

    bool operator==(const CellAttributes &a) const;
    bool operator!=(const CellAttributes &a) const;
};

struct CellRange: CellAttributes
{
    uint32_t start;
    uint32_t end;

    CellRange(const uint32_t *ptr);
};

struct Cell: CellAttributes
{
    uint32_t startptr;
    uint32_t endptr;
    uint32_t cellx;
    uint32_t cellwidth;

    Cell();
    Cell(const CellAttributes &a);
};

struct DisplayCell: CellAttributes
{
    std::string substr;
    QString text;
    QRectF rect;
    QPointF point;
    Tsq::LineFlags lineFlags;
    int data;

    DisplayCell();
    DisplayCell(const CellAttributes &a);
};

struct CellRow
{
    std::string str;
    std::vector<Cell> cells;
    std::vector<CellRange> ranges;
    Tsq::LineFlags flags;
    uint32_t size;
    int32_t modtime;
    uint32_t regionState;
    uint32_t matchStart;
    uint32_t matchEnd;

    inline CellRow() :
        flags(0),
        size(0),
        modtime(INVALID_MODTIME),
        regionState(0)
        {}

    inline void clear() {
        str.clear();
        cells.clear();
        ranges.clear();
        flags = 0;
        size = 0;
    }
};

struct CellState
{
    Tsq::CellFlags flags;
    Tsq::CellFlags invisibleFlags;
    QRgb fg;
    QRgb bg;
    qreal scale;

    inline CellState(Tsq::CellFlags invisibleFlags);
};

//
// Inlines
//

// CellAttributes32
inline
CellAttributes32::CellAttributes32() :
    flags(0), fg(0), bg(0)
{}

inline bool
CellAttributes32::operator==(const CellAttributes32 &a) const
{
    return flags == a.flags && fg == a.fg && bg == a.bg;
}

inline bool
CellAttributes32::operator!=(const CellAttributes32 &a) const
{
    return flags != a.flags || fg != a.fg || bg != a.bg;
}

// CellAttributes
inline
CellAttributes::CellAttributes() :
    flags(0), fg(0), bg(0), link(INVALID_REGION_ID)
{}

inline
CellAttributes::CellAttributes(const uint32_t *ptr) :
    flags(ptr[2]), fg(ptr[3]), bg(ptr[4]), link(ptr[5])
{}

inline
CellAttributes::CellAttributes(const CellAttributes32 &a) :
    flags(a.flags), fg(a.fg), bg(a.bg), link(INVALID_REGION_ID)
{}

inline bool
CellAttributes::isVisible(QChar val) const
{
    return (val != 0x20) || (flags & Tsqt::Visible);
}

inline bool
CellAttributes::operator==(const CellAttributes &a) const
{
    return flags == a.flags && fg == a.fg && bg == a.bg && link == a.link;
}

inline bool
CellAttributes::operator!=(const CellAttributes &a) const
{
    return flags != a.flags || fg != a.fg || bg != a.bg || link != a.link;
}

// CellRange
inline
CellRange::CellRange(const uint32_t *ptr) :
    CellAttributes(ptr), start(ptr[0]), end(ptr[1])
{}

// Cell
inline
Cell::Cell() :
    startptr(0), endptr(0), cellx(0), cellwidth(0)
{}

inline
Cell::Cell(const CellAttributes &a) :
    CellAttributes(a), startptr(0), endptr(0), cellx(0), cellwidth(0)
{}

// DisplayCell
inline
DisplayCell::DisplayCell() :
    lineFlags(0)
{}

inline
DisplayCell::DisplayCell(const CellAttributes &a) :
    CellAttributes(a)
{}

// CellState
inline
CellState::CellState(Tsq::CellFlags i) :
    flags(0), invisibleFlags(i)
{}
