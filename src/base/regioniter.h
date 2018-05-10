// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "cell.h"

class BufferBase;
class RegionBase;

//
// Build a string
//
class RegionStringBuilder
{
private:
    const BufferBase *m_buffer;
    const RegionBase *m_region;
    size_t m_size, m_cur, m_end;
    bool m_done;

    std::string m_result;

    void append(const CellRow &row, unsigned start, unsigned end);

public:
    RegionStringBuilder(const BufferBase *buffer, const RegionBase *region);

    QString build(bool oneline = false);
};
