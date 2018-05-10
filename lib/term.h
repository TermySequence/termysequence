// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "machine.h"

namespace Tsq
{
    class ProtocolCallback;
    class RawProtocol;

    class TermProtocol: public ProtocolMachine, public ProtocolCallback
    {
    private:
        char *m_inbuf, *m_binbuf;
        size_t m_insize, m_inpos;

        RawProtocol *m_submachine;

        bool m_haveEsc, m_havePrefix;

        unsigned m_upos;
        char m_ubuf[8];

        void init();
        bool process(char c);

    public:
        TermProtocol(ProtocolCallback *parent, const char *buf, size_t len);
        TermProtocol(ProtocolCallback *parent);
        ~TermProtocol();

        bool connRead(const char *buf, size_t len);
        bool connRead(int fd);

        void connSend(const char *buf, size_t len);
        void connFlush(const char *buf, size_t len);

        std::string encode(const char *buf, size_t len);

        bool protocolCallback(uint32_t command, uint32_t length, const char *body);
        void writeFd(const char *buf, size_t len);

        void reset();
    };

    class TermClientProtocol: public ProtocolMachine, public ProtocolCallback
    {
    private:
        char *m_inbuf;

        RawProtocol *m_submachine;

    public:
        TermClientProtocol(ProtocolCallback *parent);
        ~TermClientProtocol();

        bool connRead(const char *buf, size_t len);
        bool connRead(int fd);

        void connSend(const char *buf, size_t len);
        void connFlush(const char *buf, size_t len);

        bool protocolCallback(uint32_t command, uint32_t length, const char *body);
        void writeFd(const char *buf, size_t len);
    };
}
