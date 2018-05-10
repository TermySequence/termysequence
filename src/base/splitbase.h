// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settings/splitlayout.h"

class SplitWidget;

class SplitBase
{
public:
    virtual void handleSplitRequest(int type, SplitWidget *caller);
    virtual void replaceChild(SplitWidget *child, SplitWidget *replacement);
    virtual void saveLayout(SplitLayoutWriter &layout) const = 0;
};
