// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "region.h"
#include "regioniter.h"
#include "bufferbase.h"

#include <QApplication>
#include <QClipboard>

RegionBase::RegionBase(Tsqt::RegionType type, regionid_t id) :
    m_type(type),
    m_id(id)
{
}

Region::Region(Tsqt::RegionType type, BufferBase *buffer, regionid_t id) :
    RegionBase(type, id),
    m_buffer(buffer)
{
}

Region::Region(const RegionBase &other) :
    RegionBase(other),
    m_buffer(nullptr)
{
}

index_t
Region::trueEndRow() const
{
    return (endRow != INVALID_INDEX) ?
        endRow + 1 :
        m_buffer->origin() + m_buffer->size();
}

bool
RegionBase::contains(index_t row, column_t col) const
{
    if (row < startRow || row > endRow)
        return false;
    if (row == startRow) {
        if (startRow == endRow)
            return (col >= startCol) && (col < endCol);
        else
            return (col >= startCol);
    }
    if (row == endRow) {
        return (col < endCol);
    }
    return true;
}

bool
RegionBase::overlaps(const RegionBase *other) const
{
    bool a = (startRow < other->endRow) ||
        (startRow == other->endRow && startCol < other->endCol);
    bool b = (other->startRow < endRow) ||
        (other->startRow == endRow && other->startCol < endCol);

    return a && b;
}

void
Region::clipboardSelect() const
{
    QClipboard *clipboard = QApplication::clipboard();
    if (clipboard->supportsSelection()) {
        RegionStringBuilder i(m_buffer, this);
        clipboard->setText(i.build(), QClipboard::Selection);
    }
}

int
Region::clipboardCopy() const
{
    RegionStringBuilder i(m_buffer, this);
    QString text = i.build();
    QApplication::clipboard()->setText(text);
    return text.size();
}

QString
Region::getLine() const
{
    RegionStringBuilder i(m_buffer, this);
    return i.build(true);
}


bool
RegionStartSorter::operator()(const RegionRef &lhs, const RegionRef &rhs) const
{
    if (lhs.ptr == rhs.ptr)
        return false;
    if (lhs.ptr->startRow != rhs.ptr->startRow)
        return lhs.ptr->startRow < rhs.ptr->startRow;
    if (lhs.ptr->m_type != rhs.ptr->m_type)
        return lhs.ptr->m_type < rhs.ptr->m_type;
    if (lhs.ptr->startCol != rhs.ptr->startCol)
        return lhs.ptr->startCol < rhs.ptr->startCol;

    return lhs.ptr->m_id < rhs.ptr->m_id;
}

bool
RegionEndSorter::operator()(const RegionRef &lhs, const RegionRef &rhs) const
{
    if (lhs.ptr == rhs.ptr)
        return false;
    if (lhs.ptr->endRow != rhs.ptr->endRow)
        return lhs.ptr->endRow < rhs.ptr->endRow;
    if (lhs.ptr->m_type != rhs.ptr->m_type)
        return lhs.ptr->m_type < rhs.ptr->m_type;
    if (lhs.ptr->endCol != rhs.ptr->endCol)
        return lhs.ptr->endCol < rhs.ptr->endCol;

    return lhs.ptr->m_id < rhs.ptr->m_id;
}
