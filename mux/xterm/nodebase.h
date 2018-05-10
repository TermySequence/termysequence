// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/codepoint.h"

#include <unordered_map>
#include <vector>

class XTermEdge;
class XTermStateMachine;
class XTermEmulator;
typedef void (XTermEmulator::*XTermHandler)();

class XTermNode
{
private:
    XTermHandler m_slot;
    const char *m_slotName = nullptr;

public:
    XTermNode(XTermEdge *fallbackEdge);

    typedef std::unordered_map<Codepoint,XTermEdge*> EdgeMap;
    EdgeMap edgeMap;
    typedef std::vector<XTermEdge*> EdgeList;
    EdgeList edgeList;

    bool isLeaf = false;
    inline XTermHandler slot() const { return m_slot; };
    inline const char *slotName() const { return m_slotName; }

    void setSlot(XTermHandler slot) { m_slot = slot; };
    void setSlotName(const char *slotName) { m_slotName = slotName; }

    XTermEdge *process(XTermStateMachine *state, Codepoint c);
};
