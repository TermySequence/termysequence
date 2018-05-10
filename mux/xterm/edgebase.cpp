// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "edgebase.h"
#include "machine.h"

bool
XTermEdge::matches(XTermStateMachine *, Codepoint)
{
    return true;
}

XTermEdge::Disposition
XTermEdge::process(XTermStateMachine *state, Codepoint c)
{
    state->push(c);
    state->next();
    return Move;
}
