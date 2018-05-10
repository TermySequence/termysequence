// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "termcolors.h"
#include "dircolors.h"

class TermPalette final: public Termcolors, public Dircolors
{
public:
    TermPalette() = default;
    TermPalette(const QString &tc, const QString &dc);

    bool operator==(const TermPalette &other) const;
    bool operator!=(const TermPalette &other) const;
};

inline bool
TermPalette::operator==(const TermPalette &other) const
{
    return Termcolors::operator==(other) &&
        Dircolors::operator==(other);
}

inline bool
TermPalette::operator!=(const TermPalette &other) const
{
    return Termcolors::operator!=(other) ||
        Dircolors::operator!=(other);
}
