// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "mockemulator.h"

#include "mux/base/cell.cpp"
#include "mux/base/codestring.cpp"

// Implementation
const char *
TermEventTransfer::getStrIter(size_t ptr)
{
    return m_row.m_str.data() + ptr;
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
TermEventTransfer::splitChar(const char *i, unsigned pos, Tsq::Unicoding *wl)
{
    return m_row.splitChar(i, pos, wl);
}

void
TermEventTransfer::removeChar(const char *i, unsigned pos, Tsq::Unicoding *wl)
{
    m_row.removeChar(i, pos, wl);
}

void
TermEventTransfer::mergeChars(const char *i, unsigned pos, Tsq::Unicoding *wl)
{
    m_row.mergeChars(i, pos, wl);
}
