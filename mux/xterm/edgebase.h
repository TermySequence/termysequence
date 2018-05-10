// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/codestring.h"

#define EDGE_SINGLE_NUMERIC '\xff'
#define EDGE_MULTI_NUMERIC  '\xfe'
#define EDGE_SINGLE_TEXT    '\xfd'
#define EDGE_SINGLE_CHAR    '\xfc'
#define EDGE_OTHER          '\0'

class XTermNode;
class XTermStateMachine;

class XTermEdge
{
protected:
    int m_varnum = 0;

public:
    virtual ~XTermEdge() = default;

    virtual char type() { return EDGE_OTHER; };

    enum Disposition { Stay, Move, Skip, Restart, Reset, Call };

    XTermNode *next = nullptr;

    virtual bool matches(XTermStateMachine *state, Codepoint c);
    virtual Disposition process(XTermStateMachine *state, Codepoint c);
};
