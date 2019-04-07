// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "types.h"
#include "flags.h"

namespace Tsq
{
    // Codepoint range lookup table
    template<class T, T defval>
    class Unitable
    {
    private:
        const T *const m_data;
        const size_t m_size;

    public:
        Unitable(const T *data, size_t size) : m_data(data), m_size(size) {}

        CellFlags search(codepoint_t c) const;
        CellFlags lookup(codepoint_t c) const;
    };

    template<class T, T defval> CellFlags
    Unitable<T,defval>::search(codepoint_t c) const
    {
        size_t len = m_size;
        auto *start = m_data;

        do {
            // Check midpoint
            size_t half = len / 2;
            auto *entry = start + 3 * half;
            if (entry[0] > c) {
                len = half;
            } else if (entry[1] < c) {
                ++half;
                start += 3 * half;
                len -= half;
            } else {
                return entry[2];
            }
        } while (len);

        return defval;
    }

    template<class T, T defval> inline CellFlags
    Unitable<T,defval>::lookup(codepoint_t c) const
    {
        return c >= m_data[0] ? search(c) : defval;
    }
}
