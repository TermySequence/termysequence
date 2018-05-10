// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "conn.h"
#include "clientmachine.h"

class RawInstance final: public ConnInstance
{
    friend class ClientMachine;

private:
    int m_protocolType;
    int m_newFd;
    bool m_indirect;

    bool setRemoteId(const Tsq::Uuid &remoteId);
    void writeResponse(const char *buf, size_t len);
    bool setMachine(Tsq::ProtocolMachine *newMachine, AttributeMap &attributes);

    void threadMain();
    bool handleFd();
    bool handleWork(const WorkItem &item);
    bool handleIdle();

    void postConnect();
    void postDisconnect(bool locked);

public:
    RawInstance(int protocolType, int fd, const char *buf, size_t len);
    RawInstance(int protocolType, int fd, int newFd);

    inline int fd() { return m_fd; }
};
