// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/palette.h"

#include <array>

class TermPalette final: public std::array<uint32_t,PALETTE_SIZE>
{
public:
    TermPalette(const std::string &spec);

    void parse(const std::string &spec);
    std::string toString();
};
