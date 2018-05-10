// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "edgecontrol.h"
#include "machine.h"

bool
XTermControlEdge::matches(XTermStateMachine *, Codepoint c)
{
    return XTermStateMachine::isControlCode(c);
}

XTermEdge::Disposition
XTermControlEdge::process(XTermStateMachine *state, Codepoint c)
{
    XTermEdge::Disposition rc;

    if (XTermStateMachine::isRestartCode(c)) {
        rc = state->allSequence().empty() ? Move : Restart;
    } else {
        rc = Call;
    }

    state->push(c);
    // Don't start a new sequence due to embedded control chars
    // state->next();
    return rc;
}
