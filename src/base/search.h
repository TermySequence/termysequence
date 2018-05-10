// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/enums.h"

#include <QString>
#include <regex>

struct TermSearch
{
    bool active;
    bool matchCase;
    Tsqt::SearchType type;

    QString text;
    std::regex regex;

    inline TermSearch();
    inline bool operator!=(const TermSearch &o) const;
};

inline TermSearch::TermSearch() :
    active(false), matchCase(false), type(Tsqt::SingleLinePlainText)
{
}

inline bool
TermSearch::operator!=(const TermSearch &o) const
{
    return active != o.active || matchCase != o.matchCase || type != o.type || text != o.text;
}
