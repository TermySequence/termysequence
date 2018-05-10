// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "portfwdtask.h"

class PortInTask final: public PortFwdTask
{
    Q_OBJECT

private:
    int connectHelper(PortFwdState &state, struct addrinfo *p);
    int connectTcp(PortFwdState &state);
    int connectUnix(PortFwdState &state);

    void handleStart(Tsq::ProtocolUnmarshaler *unm);

private slots:
    void handleConnect(int fd);

public:
    PortInTask(ServerInstance *server, const PortFwdRule &config);

    void start(TermManager *manager);

    TermTask* clone() const;
};
