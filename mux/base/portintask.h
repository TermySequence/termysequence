// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "portfwdtask.h"

class PortIn final: public PortBase
{
private:
    portfwd_t m_nextId = INVALID_PORTFWD;

    bool listenTcp();
    bool listenUnix();
    void acceptfd(pollfd &pfd, PortFwdState *cstate);

    bool handleMultiFd(pollfd &pfd);
    void threadMain();

public:
    PortIn(Tsq::ProtocolUnmarshaler *unm);
};
