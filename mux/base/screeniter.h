// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "cell.h"

class TermScreen;

//
// Iterate over rows
//
class TermRowIterator
{
private:
    TermScreen *m_screen;
    int m_cur, m_end;

public:
    TermRowIterator(TermScreen *screen);

    int y;
    CellRow& row();
    CellRow& singleRow();

    void next();
    bool done;
};
