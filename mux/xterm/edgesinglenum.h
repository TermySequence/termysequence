// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "edgebase.h"

class XTermSingleNumericEdge final: public XTermEdge
{
private:
    CodepointList m_terminators;

public:
    XTermSingleNumericEdge(int varnum, Codepoint terminator);
    char type() { return EDGE_SINGLE_NUMERIC; };

    void addTerminator(Codepoint terminus);

    bool matches(XTermStateMachine *state, Codepoint c);
    Disposition process(XTermStateMachine *state, Codepoint c);
};
