// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "tabstops.h"

#define DEFAULT_WIDTH 132

TermTabStops::TermTabStops(int width): m_width(width)
{
    int n = (width > DEFAULT_WIDTH) ? width : DEFAULT_WIDTH;

    push_back(false);

    for (int i = 1; i < n; ++i)
        push_back((i % 8) == 0);
}

void
TermTabStops::reset()
{
    (*this)[0] = false;

    for (int i = 1; i < m_width; ++i)
        (*this)[i] = (i % 8) == 0;
}

void
TermTabStops::setWidth(int width)
{
    m_width = width;

    while (m_width > size())
        push_back(false);
}

void
TermTabStops::setTabStop(int pos)
{
    (*this)[pos] = true;
}

void
TermTabStops::clearTabStop(int pos)
{
    (*this)[pos] = false;
}

void
TermTabStops::clearTabStops()
{
    for (int i = 0; i < size(); ++i)
        if (at(i))
            (*this)[i] = false;
}

int
TermTabStops::getNextTabStop(int pos)
{
    for (int i = pos + 1; i < m_width; ++i)
        if (at(i))
            return i;

    return m_width - 1;
}

int
TermTabStops::getPrevTabStop(int pos)
{
    for (int i = pos - 1; i > 0; --i)
        if (at(i))
            return i;

    return 0;
}

