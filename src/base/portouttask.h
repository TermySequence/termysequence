// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "portfwdtask.h"

class PortOutTask final: public PortFwdTask
{
private:
    bool listenTcp(TermManager *manager);
    bool listenUnix(TermManager *manager);

public:
    PortOutTask(ServerInstance *server, const PortFwdRule &config);

    void start(TermManager *manager);

    TermTask* clone() const;
};
