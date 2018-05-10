// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "types.h"

#include <set>
#include <initializer_list>

namespace Tsq
{
    typedef std::pair<codepoint_t,codepoint_t> Unirange;

    // Codepoint range set
    class Uniset
    {
    private:
        struct RangeSorter
        {
            bool operator()(const Unirange &a, const Unirange &b) const {
                return a.second < b.first;
            }
        };
        std::set<Unirange,RangeSorter> m_set;
        codepoint_t m_lower;

    public:
        Uniset(std::initializer_list<Unirange> l): m_set(l) {
            m_lower = m_set.begin()->first;
        }

        // Checks lower bound
        inline bool has(Unirange r) const {
            return r.first >= m_lower && m_set.count(r);
        }
        inline bool has(codepoint_t c) const {
            return c >= m_lower && m_set.count(std::make_pair(c, c));
        }
        // Doesn't check lower bound (goes straight to set)
        inline bool contains(Unirange r) const {
            return m_set.count(r);
        }
        inline bool contains(codepoint_t c) const {
            return m_set.count(std::make_pair(c, c));
        }
    };
}
