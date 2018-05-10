// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <vector>

class TermTabStops final: public std::vector<bool>
{
private:
    int m_width;

public:
    TermTabStops(int width);
    void reset();
    void setWidth(int width);

    void setTabStop(int pos);
    void clearTabStop(int pos);
    void clearTabStops();

    int getNextTabStop(int pos);
    int getPrevTabStop(int pos);
};
