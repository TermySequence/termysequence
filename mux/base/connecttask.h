// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "taskbase.h"
#include "connectbase.h"
#include "attributemap.h"

class RunConnect final: public TaskBase, public ConnectorBase
{
private:
    StringMap m_params;

private:
    bool begin();
    void reportStarting();
    void reportStatus(Tsq::TaskStatus status, const char *buf, unsigned len);
    void reportError();

    void threadMain();
    bool handleInterrupt();
    bool handleMultiFd(pollfd &pfd);
    bool readRemoteHandshake(pollfd &pfd);
    bool writeRemoteHandshake(pollfd &pfd);
    void localConnect();

    bool handleWork(const WorkItem &item);
    void handleCancel();
    bool handleData(std::string *data);

public:
    RunConnect(Tsq::ProtocolUnmarshaler *unm);
};
