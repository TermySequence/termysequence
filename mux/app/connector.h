// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/threadbase.h"
#include "base/connectbase.h"

class TermConnector final: public ThreadBase, public ConnectorBase
{
private:
    char *m_termios = nullptr;

    bool readRemoteHandshake(pollfd &pfd);
    bool writeRemoteHandshake(pollfd &pfd);
    bool localConnect();

    void threadMain();
    bool handleMultiFd(pollfd &pfd);

public:
    TermConnector(int pid, bool pty, bool raw);
    ~TermConnector();
};
