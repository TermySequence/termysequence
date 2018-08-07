// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "taskbase.h"
#include "attributemap.h"

class RunCommand final: public TaskBase
{
private:
    char *m_buf, *m_ptr;
    uint32_t m_chunkSize, m_windowSize;
    size_t m_sent = 0;
    size_t m_acked = 0;
    size_t m_received = 0;
    size_t m_chunks = 0;

    bool m_running = false;
    bool m_exited = false;
    int m_disposition;

    std::queue<std::string*> m_outdata;

    StringMap m_params;

private:
    bool begin();
    void reportStarting();
    void reportAck();
    void reportFinished();
    void reportError(int errtype, int errnum);

    void threadMain();
    bool handleInterrupt();

    bool handleMultiFd(pollfd &pfd);
    bool handleRead(pollfd &pfd);
    void handleWrite(pollfd &pfd);

    bool handleWork(const WorkItem &item);
    void handleCancel();

    void handleData(std::string *data);

public:
    RunCommand(Tsq::ProtocolUnmarshaler *unm);
    ~RunCommand();
};
