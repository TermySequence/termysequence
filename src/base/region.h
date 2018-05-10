// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"
#include "app/enums.h"
#include "lib/types.h"
#include "lib/flags.h"

#include <vector>

QT_BEGIN_NAMESPACE
class QSvgRenderer;
QT_END_NAMESPACE
class BufferBase;
class SemanticParser;
class SemanticFlash;

//
// Base class - stores bounds, type, and id only
//
class RegionBase
{
    friend struct RegionStartSorter;
    friend struct RegionEndSorter;

private:
    Tsqt::RegionType m_type;
    regionid_t m_id;

public:
    index_t startRow = INVALID_INDEX;
    index_t endRow = INVALID_INDEX;
    column_t startCol = INVALID_COLUMN;
    column_t endCol = INVALID_COLUMN;

public:
    RegionBase(Tsqt::RegionType type, regionid_t id = INVALID_REGION_ID);

    Tsqt::RegionType type() const { return m_type; }
    regionid_t id() const { return m_id; }

    inline bool isEmpty() const { return startRow == endRow && startCol == endCol; }

    inline bool contains(index_t row) const { return row >= startRow && row <= endRow; }
    bool contains(index_t row, column_t col) const;

    bool overlaps(const RegionBase *other) const;
};

//
// Region class - stores parent, flags, buffer, and attributes
//
class Region: public RegionBase
{
public:
    regionid_t parent;
    Tsq::RegionFlags flags;
    AttributeMap attributes;

    union {
        SemanticParser *parser = nullptr;
        SemanticFlash *flash;
        QSvgRenderer *renderer;
    };

protected:
    BufferBase *m_buffer;

public:
    Region(Tsqt::RegionType type, BufferBase *buffer, regionid_t id);
    Region(const RegionBase &other);

    const BufferBase* buffer() const { return m_buffer; }
    index_t trueEndRow() const;

    void clipboardSelect() const;
    int clipboardCopy() const;
    QString getLine() const;
};

//
// Ancillary classes
//
struct RegionRef
{
    RegionBase *ptr;
    inline RegionRef(RegionBase *p): ptr(p) {}
    inline Region* ref() const { return static_cast<Region*>(ptr); }
    inline const Region* cref() const { return static_cast<Region*>(ptr); }
};

struct RegionList
{
    std::vector<RegionRef> list;
};

struct RegionStartSorter
{
    bool operator()(const RegionRef &lhs, const RegionRef &rhs) const;
};

struct RegionEndSorter
{
    bool operator()(const RegionRef &lhs, const RegionRef &rhs) const;
};
