// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cell.h"

CellAttributes32::CellAttributes32(std::initializer_list<unsigned> l)
{
    auto i = l.begin();
    flags = *i++;
    fg = *i++;
    bg = *i;
}
