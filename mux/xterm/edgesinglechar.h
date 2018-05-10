// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "edgebase.h"

class XTermSingleCharEdge final: public XTermEdge
{
public:
    XTermSingleCharEdge(int varnum);
    char type() { return EDGE_SINGLE_CHAR; };

    Disposition process(XTermStateMachine *state, Codepoint c);
};
