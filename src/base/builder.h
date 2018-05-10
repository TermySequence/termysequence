// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/config.h"
#include "cell.h"

class CellBuilder final: public Cell
{
private:
    CellRow &row;
    int startpos;
    int endpos;
    int x;

    void push();

public:
    CellBuilder(CellRow &row);

    void visit(const CellAttributes &a, bool visible, int pos, int startp, int endp);
    void finish();
};

inline
CellBuilder::CellBuilder(CellRow &r) :
    row(r), startpos(-1), x(0)
{}

inline void
CellBuilder::push()
{
    cellwidth = (endpos - startpos + 1) * (1 + !!(flags & Tsq::DblWidthChar));
    row.cells.emplace_back(*this);
}

inline void
CellBuilder::visit(const CellAttributes &a, bool visible, int pos,
                   int startp, int endp)
{
    if (startpos == -1) {
        if (visible) {
            // start a new cell
            CellAttributes::operator=(a);
            startptr = startp;
            endptr = endp;
            startpos = endpos = pos;
            cellx = x;
        }
    }
    else if (!visible) {
        if (pos - endpos > CELL_SPLIT_THRESHOLD ||
            (flags & (Tsqt::Visible|Tsq::DblWidthChar|Tsq::EmojiChar))) {
            // too many spaces or a mismatch, write out the current cell
            push();
            startpos = -1;
        }
    }
    else if (a != *this) {
        // no match, write out the current cell
        push();
        // start a new cell
        CellAttributes::operator=(a);
        startptr = startp;
        endptr = endp;
        startpos = endpos = pos;
        cellx = x;
    }
    else {
        // add to current cell
        endptr = endp;
        endpos = pos;
    }

    x += 1 + !!(a.flags & Tsq::DblWidthChar);
}

inline void
CellBuilder::finish()
{
    if (startpos != -1)
        push();
}
