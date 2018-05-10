// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "edgesingletext.h"
#include "machine.h"
#include "config.h"

XTermSingleTextEdge::XTermSingleTextEdge(int varnum, Codepoint terminator)
{
    m_varnum = varnum;
    m_terminators.push_back(terminator);
}

void
XTermSingleTextEdge::addTerminator(Codepoint terminator)
{
    m_terminators.push_back(terminator);
}

bool
XTermSingleTextEdge::matches(XTermStateMachine *state, Codepoint c)
{
    if (m_terminators.contains(c)) {
        return true;
    }
    if (state->curSequence().str().size() > SEQUENCE_FIELD_MAX) {
        return false;
    }
    return !XTermStateMachine::isRestartCode(c);
}

XTermEdge::Disposition
XTermSingleTextEdge::process(XTermStateMachine *state, Codepoint c)
{
    if (m_terminators.contains(c)) {
        state->pushVar(m_varnum, state->curSequence());
        state->next();
        return Skip;
    }

    state->push(c);
    return Stay;
}
