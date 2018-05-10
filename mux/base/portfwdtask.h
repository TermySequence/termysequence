// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/types.h"
#include "taskbase.h"

#include <unordered_map>

class PortBase: public TaskBase
{
protected:
    struct PortFwdState {
        portfwd_t id;
        int fd;
        std::queue<std::string*> outdata;
        struct addrinfo *a, *p;
        bool special;
    };

    char *m_buf, *m_ptr;
    uint32_t m_chunkSize, m_windowSize;

    size_t m_received = 0, m_chunks = 0; // from client
    size_t m_sent = 0, m_acked = 0; // to client

    std::unordered_map<int,PortFwdState*> m_fdmap;
    std::unordered_map<portfwd_t,PortFwdState*> m_idmap;

    bool m_running = true;

    Tsq::PortForwardTaskType m_type;
    std::string m_address, m_port;

protected:
    void pushStart(portfwd_t id, const char *host, const char *serv);
    void pushBytes(portfwd_t id, size_t len);
    void pushAck();

    void watchReads(bool enabled);
    void watchWrites(int fd, bool enabled);

    void closefd(portfwd_t id);
    void readfd(pollfd &pfd, PortFwdState *cstate);
    void writefd(pollfd &pfd, PortFwdState *cstate);

    virtual void handleStart(portfwd_t id);
    bool handleBytes(portfwd_t id, std::string *data);
    void handleData(std::string *data);
    bool handleWork(const WorkItem &item);

public:
    PortBase(const char *name, Tsq::ProtocolUnmarshaler *unm, unsigned flags);
    ~PortBase();
};
