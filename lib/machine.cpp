// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "machine.h"
#include "exception.h"

namespace Tsq
{
    ProtocolMachine::ProtocolMachine(ProtocolCallback *parent, const char *buf, size_t len) :
        m_parent(parent),
        m_buf(nullptr),
        m_len(len)
    {
        if (len)
            memcpy(m_buf = new char[len], buf, len);
    }

    ProtocolMachine::ProtocolMachine(ProtocolCallback *parent) :
        m_parent(parent),
        m_buf(nullptr),
        m_len(0)
    {
    }

    ProtocolMachine::~ProtocolMachine()
    {
        delete [] m_buf;
    }

    bool
    ProtocolMachine::start()
    {
        if (!m_len)
            return true;

        bool rc = connRead(m_buf, m_len);
        delete [] m_buf;
        m_buf = nullptr;
        return rc;
    }

    void
    ProtocolMachine::reset()
    {
    }

    void
    ProtocolMachine::connSend(const char *buf, size_t len)
    {
        m_parent->writeFd(buf, len);
    }

    void
    ProtocolMachine::connFlush(const char *buf, size_t len)
    {
        m_parent->writeFd(buf, len);
    }

    std::string
    ProtocolMachine::encode(const char *buf, size_t len)
    {
        return std::string(buf, len);
    }

    void
    ProtocolCallback::eofCallback(int errnum)
    {
        if (errnum)
            throw ErrnoException("read", errnum);
    }
}
