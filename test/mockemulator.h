// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/unicode.h"
#include "mux/base/cell.h"

// Provides friend access to CellRow
struct TermEventTransfer {
    CellRow &m_row;
    inline TermEventTransfer(CellRow &row): m_row(row) {}

    inline const auto& ranges() const { return m_row.m_ranges; }
    inline auto clusters() { return m_row.m_clusters; }
    const char* getStrIter(size_t ptr);

    void setStr(const char *str, unsigned clusters, int width);
    void setRanges(const uint32_t *data, size_t nmemb);

    size_t splitChar(const char *i, unsigned pos, Tsq::Unicoding *wl);
    void removeChar(const char *i, unsigned pos, Tsq::Unicoding *wl);
    void mergeChars(const char *i, unsigned pos, Tsq::Unicoding *wl);
};
