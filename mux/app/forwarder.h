// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/threadbase.h"
#include "lib/uuid.h"
#include "lib/handshake.h"

class TermForwarder;

/*
 * ForwarderWriter
 */
class TermForwarderWriter final: public ThreadBase
{
private:
    int m_infd, m_outfd;
    char *m_buf, *m_ptr = nullptr;
    size_t m_ptrlen = 0;

    TermForwarder *m_parent;
    int m_code;

    void threadMain();
    bool handleMultiFd(pollfd &pfd);

public:
    TermForwarderWriter(TermForwarder *parent, int infd, int outfd, int code);
    ~TermForwarderWriter();
};

/*
 * Forwarder
 */
class TermForwarder final: public ThreadBase
{
private:
    Tsq::ClientHandshake m_handshake{Tsq::Uuid::mt.buf, true};
    std::string m_outbuf;
    int m_sd[2];
    char *m_termios = nullptr;

    TermForwarderWriter *m_reader = nullptr;
    TermForwarderWriter *m_writer = nullptr;

    int m_protocolType;
    int m_state;
    bool m_needsForwarding;
    bool m_cleanExit = false;
    bool m_writerExit = false;

    void readHandshake();
    void writeHandshake();
    void readResponse();
    void writeDescriptors();
    bool readInfo();
    void writeDisconnect();

    void threadMain();
    bool handleInterrupt();
    bool handleWork(const WorkItem &item);
    bool handleMultiFd(pollfd &pfd);

public:
    TermForwarder();
    ~TermForwarder();
};

enum ForwarderWork {
    ForwarderError,
    ForwarderReaderExit,
    ForwarderWriterExit,
};
