// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "edgesinglechar.h"
#include "machine.h"

XTermSingleCharEdge::XTermSingleCharEdge(int varnum)
{
    m_varnum = varnum;
}

XTermEdge::Disposition
XTermSingleCharEdge::process(XTermStateMachine *state, Codepoint c)
{
    state->pushVar(m_varnum, c);
    state->push(c);
    state->next();
    return Move;
}
