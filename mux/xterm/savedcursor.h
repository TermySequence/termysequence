// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

struct XTermSavedCursor
{
    Tsq::TermFlags flags;
    CursorBase cursor;
    CellAttributes attributes;
    int gl, gr, nextgl;
    std::array<Charset,4> charsets;
};
