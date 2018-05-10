// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

namespace Tsq
{
    class ProtocolCallback
    {
    public:
        virtual bool protocolCallback(uint32_t command, uint32_t length, const char *body) = 0;
        virtual void writeFd(const char *buf, size_t len) = 0;

        virtual void eofCallback(int errnum);
    };

    class ProtocolMachine
    {
    protected:
        ProtocolCallback *m_parent;

    private:
        char *m_buf;
        size_t m_len;

    public:
        ProtocolMachine(ProtocolCallback *parent, const char *buf, size_t len);
        ProtocolMachine(ProtocolCallback *parent);
        virtual ~ProtocolMachine();

        virtual bool connRead(const char *buf, size_t len) = 0;
        virtual bool connRead(int fd) = 0;

        virtual void connSend(const char *buf, size_t len);
        virtual void connFlush(const char *buf, size_t len);

        virtual std::string encode(const char *buf, size_t len);

        virtual bool start();
        virtual void reset();
    };
}
