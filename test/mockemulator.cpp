// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "mockemulator.h"

#include "mux/base/cell.cpp"
#include "mux/base/codestring.cpp"

// Implementation
std::string::iterator
TermEventTransfer::getStrIter(size_t ptr)
{
    auto i = m_row.m_str.begin() + ptr;
    return i;
}

void
TermEventTransfer::setStr(const char *str, unsigned clusters, int width)
{
    m_row.m_str = str;
    m_row.m_clusters = clusters;
    m_row.m_columns = width;
}

void
TermEventTransfer::setRanges(const uint32_t *data, size_t nmemb)
{
    m_row.m_ranges.assign(data, data + nmemb);
}

size_t
TermEventTransfer::splitChar(std::string::iterator i, unsigned pos, Tsq::Unicoding *wl)
{
    return m_row.splitChar(i, pos, wl);
}

void
TermEventTransfer::removeChar(std::string::iterator i, unsigned pos, Tsq::Unicoding *wl)
{
    m_row.removeChar(i, pos, wl);
}

void
TermEventTransfer::mergeChars(std::string::iterator i, unsigned pos, Tsq::Unicoding *wl)
{
    m_row.mergeChars(i, pos, wl);
}
