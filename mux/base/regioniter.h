// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"

namespace Tsq { class Unicoding; }
class TermBuffer;
class Region;

//
// Build a string
//
class RegionStringBuilder
{
private:
    const TermBuffer *m_buffer;
    Tsq::Unicoding *m_lookup;
    const Region *m_region;
    index_t m_size, m_cur, m_end;
    unsigned m_maxLines;
    bool m_done = false;

    std::string m_result;

public:
    RegionStringBuilder(const Region *region, const TermBuffer *buffer,
                        unsigned maxLines);

    std::string& build();
};
