// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "region.h"
#include "buffer.h"
#include "screen.h"

Region::Region(Tsq::RegionType t) :
    parent(INVALID_REGION_ID),
    type(t)
{
}

Region::Region(Tsq::RegionType t, regionid_t p) :
    parent(p),
    type(t)
{
}

bool
Region::overlaps(const Region *other) const
{
    bool a = (startRow < other->endRow) ||
        (startRow == other->endRow && startCol < other->endCol);
    bool b = (other->startRow < endRow) ||
        (other->startRow == endRow && other->startCol < endCol);

    return a && b;
}

bool
Region::putReference()
{
    if (--refcount == 0) {
        delete this;
        return false;
    } else {
        return true;
    }
}

void
Region::begin(TermBuffer *buffer, const TermScreen *screen)
{
    startRow = screen->offset() + screen->cursor().y();
    startCol = screen->cursor().pos();

    endRow = INVALID_INDEX;
    endCol = 0;

    flags = Tsq::HasStart;

    buffer->beginRegion(this);
}

void
Region::beginAtX(TermBuffer *buffer, const TermScreen *screen)
{
    startRow = screen->offset() + screen->cursor().y();
    startCol = screen->cursor().x();

    endRow = INVALID_INDEX;
    endCol = 0;

    flags = Tsq::HasStart;

    buffer->beginRegion(this);
}

void
Region::end(TermBuffer *buffer, const TermScreen *screen)
{
    endRow = screen->offset() + screen->cursor().y();
    endCol = screen->cursor().pos();

    if (startRow > endRow) {
        endRow = startRow;
        endCol = startCol;
    } else if (startRow == endRow && startCol > endCol) {
        endCol = startCol;
    }

    flags |= Tsq::HasEnd;

    buffer->endRegion(this);
}

void
Region::endAtX(TermBuffer *buffer, const TermScreen *screen)
{
    endRow = screen->offset() + screen->cursor().y();
    endCol = screen->cursor().x();

    flags |= Tsq::HasEnd;

    buffer->endRegion(this);
}


bool
RegionStartSorter::operator()(const RegionRef &lhs, const RegionRef &rhs) const
{
    if (lhs.ptr == rhs.ptr)
        return false;
    if (lhs.ptr->startRow != rhs.ptr->startRow)
        return lhs.ptr->startRow < rhs.ptr->startRow;
    if (lhs.ptr->type != rhs.ptr->type)
        return lhs.ptr->type < rhs.ptr->type;

    return lhs.ptr->id < rhs.ptr->id;
}

bool
RegionEndSorter::operator()(const RegionRef &lhs, const RegionRef &rhs) const
{
    if (lhs.ptr == rhs.ptr)
        return false;
    if (lhs.ptr->endRow != rhs.ptr->endRow)
        return lhs.ptr->endRow < rhs.ptr->endRow;
    if (lhs.ptr->type != rhs.ptr->type)
        return lhs.ptr->type < rhs.ptr->type;

    return lhs.ptr->id < rhs.ptr->id;
}
