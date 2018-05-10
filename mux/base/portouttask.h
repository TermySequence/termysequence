// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "portfwdtask.h"

class PortOut final: public PortBase
{
private:
    int connectHelper(PortFwdState &state, short &eventret, struct addrinfo *p);
    int connectTcp(PortFwdState &state, short &eventret);
    int connectUnix(PortFwdState &state, short &eventret);
    void connectfd(pollfd &pfd, PortFwdState *state);

    void handleStart(portfwd_t id);
    bool handleMultiFd(pollfd &pfd);
    void threadMain();

public:
    PortOut(Tsq::ProtocolUnmarshaler *unm);
};
