// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "edgemultinum.h"
#include "machine.h"

#define MAX_LENGTH 32

XTermMultiNumericEdge::XTermMultiNumericEdge(int varnum, Codepoint separator, Codepoint terminator):
    m_separator(separator)
{
    m_varnum = varnum;
    m_terminators.push_back(terminator);
}

void
XTermMultiNumericEdge::addTerminator(Codepoint terminator)
{
    m_terminators.push_back(terminator);
}

bool
XTermMultiNumericEdge::matches(XTermStateMachine *state, Codepoint c)
{
    if (m_terminators.contains(c) || c == m_separator) {
        return true;
    }
    if (state->curSequence().str().size() > MAX_LENGTH) {
        return false;
    }
    return (c >= '0' && c <= '9');
}

XTermEdge::Disposition
XTermMultiNumericEdge::process(XTermStateMachine *state, Codepoint c)
{
    const Codestring &cur = state->curSequence();

    if (m_terminators.contains(c)) {
        state->pushVar(m_varnum, cur);
        state->next();
        return Skip;
    }
    else if (c == m_separator) {
        state->pushVar(m_varnum, cur);
        state->push(c);
        state->next();
        return Stay;
    }

    state->push(c);
    return Stay;
}
