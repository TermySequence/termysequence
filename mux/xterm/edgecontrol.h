// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "edgebase.h"

class XTermControlEdge final: public XTermEdge
{
public:
    bool matches(XTermStateMachine *state, Codepoint c);
    Disposition process(XTermStateMachine *state, Codepoint c);
};
