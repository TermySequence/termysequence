// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "edgesinglenum.h"
#include "machine.h"

#define MAX_LENGTH 32

XTermSingleNumericEdge::XTermSingleNumericEdge(int varnum, Codepoint terminator)
{
    m_varnum = varnum;
    m_terminators.push_back(terminator);
}

void
XTermSingleNumericEdge::addTerminator(Codepoint terminator)
{
    m_terminators.push_back(terminator);
}

bool
XTermSingleNumericEdge::matches(XTermStateMachine *state, Codepoint c)
{
    if (m_terminators.contains(c)) {
        return true;
    }
    if (state->curSequence().str().size() > MAX_LENGTH) {
        return false;
    }
    return (c >= '0' && c <= '9');
}

XTermEdge::Disposition
XTermSingleNumericEdge::process(XTermStateMachine *state, Codepoint c)
{
    const Codestring &cur = state->curSequence();

    if (m_terminators.contains(c)) {
        state->pushVar(m_varnum, cur);
        state->next();
        return Skip;
    }

    state->push(c);
    return Stay;
}
