// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "nodebase.h"
#include "edgebase.h"

XTermNode::XTermNode(XTermEdge *fallbackEdge)
{
    edgeList.push_back(fallbackEdge);
}

XTermEdge*
XTermNode::process(XTermStateMachine *state, Codepoint c)
{
    auto i = edgeMap.find(c);
    if (i != edgeMap.end()) {
        return i->second;
    }

    for (XTermEdge *e: edgeList)
    {
        if (e->matches(state, c))
            return e;
    }

    return nullptr;
}
