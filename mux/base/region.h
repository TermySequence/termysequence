// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"
#include "lib/enums.h"
#include "cell.h"
#include "attributemap.h"

class TermBuffer;
class TermScreen;

class Region
{
public:
    regionid_t id;
    regionid_t parent;
    uint16_t refcount = 1;
    uint8_t type;
    uint8_t bufid;
    Tsq::RegionFlags flags = 0;

    index_t startRow;
    index_t endRow;
    column_t startCol;
    column_t endCol;

    StringMap attributes;

public:
    Region(Tsq::RegionType type);
    Region(Tsq::RegionType type, regionid_t parent);

    bool overlaps(const Region *other) const;
    inline unsigned wireType() const { return type << 8 | bufid; }
    inline bufreg_t bufreg() const { return MAKE_BUFREG(bufid, id); }
    inline bufreg_t pbufreg() const { return MAKE_BUFREG(bufid, parent); }

    inline void takeReference() { ++refcount; }
    bool putReference();

    void begin(TermBuffer *buffer, const TermScreen *screen);
    void end(TermBuffer *buffer, const TermScreen *screen);
    void beginAtX(TermBuffer *buffer, const TermScreen *screen);
    void endAtX(TermBuffer *buffer, const TermScreen *screen);
};

struct RegionRef
{
    Region *ptr;
    inline RegionRef(Region *p): ptr(p) {}
};

struct RegionStartSorter
{
    bool operator()(const RegionRef &lhs, const RegionRef &rhs) const;
};

struct RegionEndSorter
{
    bool operator()(const RegionRef &lhs, const RegionRef &rhs) const;
};
