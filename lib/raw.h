// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "machine.h"

namespace Tsq
{
    class ProtocolCallback;

    class RawProtocol: public ProtocolMachine
    {
    private:
        char *m_inbuf, *m_outbuf;
        size_t m_insize, m_inpos;
        size_t m_outsize, m_outpos;

        uint32_t m_command;
        uint32_t m_payloadlength;
        uint32_t m_length;
        bool m_haveHeader;
        bool m_haveBody;

        void init();

    public:
        RawProtocol(ProtocolCallback *parent, const char *buf, size_t len);
        RawProtocol(ProtocolCallback *parent);
        ~RawProtocol();

        bool connRead(const char *buf, size_t len);
        bool connRead(int fd);
        // Low-level access for callers needing residual
        int connProcess(const char *buf, size_t len);

        void connSend(const char *buf, size_t len);
        void connFlush(const char *buf, size_t len);

        void reset();
    };
}
