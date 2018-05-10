// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "palette.h"
#include "lib/defcolors.h"

#include <sstream>

TermPalette::TermPalette(const std::string &str)
{
    parse(str);
}

void
TermPalette::parse(const std::string &str)
{
    for (unsigned i = 0; i < PALETTE_SIZE; ++i)
        (*this)[i] = s_defaultPalette[i];

    size_t cur = 0;
    bool done = false;

    do {
        size_t idx = str.find(',', cur);
        if (idx == std::string::npos)
            break;

        std::string colorNum = str.substr(cur, idx - cur);
        cur = idx + 1;
        idx = str.find(',', cur);

        if (idx == std::string::npos) {
            idx = str.size();
            done = true;
        }

        std::string colorSpec = str.substr(cur, idx - cur);
        cur = idx + 1;

        // Parse the number
        const char *startptr = colorNum.c_str();
        char *endptr;
        long num = strtol(startptr, &endptr, 16);
        if (!*startptr || *endptr || num < 0)
            break;

        // Parse the spec
        startptr = colorSpec.c_str();
        unsigned long color = strtol(startptr, &endptr, 16);
        if (!*startptr || *endptr)
            break;

        if (num < PALETTE_SIZE)
            (*this)[num] = color & PALETTE_VALUEMASK;
    } while (!done);
}

std::string
TermPalette::toString()
{
    std::ostringstream result;
    bool empty = true;

    for (unsigned i = 0; i < PALETTE_SIZE; ++i) {
        unsigned spec = (*this)[i];
        if (spec != s_defaultPalette[i]) {
            if (empty)
                empty = false;
            else
                result << ',';

            result << std::hex << i << ',' << std::hex << spec;
        }
    }

    return result.str();
}
