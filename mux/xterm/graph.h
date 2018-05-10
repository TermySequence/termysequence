// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "nodebase.h"
#include "edgecontrol.h"

#include <vector>

class XTermRootNode final: public XTermNode
{
private:
    XTermControlEdge e_control;

    // Used by destructor only
    std::vector<XTermNode*> m_nodes;
    std::vector<XTermEdge*> m_edges;

    XTermNode* addLiteralEdge(XTermNode *cur, Codepoint val);
    XTermNode* addSingleCharEdge(XTermNode *cur, int varnum);
    XTermNode* addSingleNumericEdge(XTermNode *cur, int varnum, Codepoint terminator);
    XTermNode* addMultiNumericEdge(XTermNode *cur, int varnum, Codepoint separator, Codepoint terminator);
    XTermNode* addSingleTextEdge(XTermNode *cur, int varnum, Codepoint terminator);

    void addCommand(XTermNode *root, const struct command *command);

public:
    XTermRootNode();
    ~XTermRootNode();
};
