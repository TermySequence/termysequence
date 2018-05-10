// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "edgebase.h"

class XTermMultiNumericEdge final: public XTermEdge
{
private:
    Codepoint m_separator;
    CodepointList m_terminators;

public:
    XTermMultiNumericEdge(int varnum, Codepoint separator, Codepoint terminator);
    char type() { return EDGE_MULTI_NUMERIC; };

    void addTerminator(Codepoint terminus);

    bool matches(XTermStateMachine *state, Codepoint c);
    Disposition process(XTermStateMachine *state, Codepoint c);
};
