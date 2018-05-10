// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "term.h"
#include "raw.h"
#include "endian.h"
#include "config.h"
#include "exception.h"
#include "base64.h"
#include "sequences.h"

#include <unistd.h>

namespace Tsq
{
    TermClientProtocol::TermClientProtocol(ProtocolCallback *parent) :
        ProtocolMachine(parent),
        m_inbuf(new char[BODY_DEF_LENGTH]),
        m_submachine(new RawProtocol(this))
    {
    }

    TermClientProtocol::~TermClientProtocol()
    {
        delete [] m_inbuf;
    }

    bool
    TermClientProtocol::connRead(const char *buf, size_t len)
    {
        if (len > BODY_DEF_LENGTH || !::is_base64_str(buf, len))
            throw ProtocolException(EPROTO);

        size_t rc = ::unbase64(buf, len, m_inbuf);
        return m_submachine->connRead(m_inbuf, rc);
    }

    bool
    TermClientProtocol::connRead(int fd)
    {
        ssize_t got;
        char buf[READER_BUFSIZE];

        switch (got = read(fd, buf, sizeof(buf))) {
        default:
            return connRead(buf, got);
        case 0:
            m_parent->eofCallback(0);
            return false;
        case -1:
            if (errno == EINTR || errno == EAGAIN)
                return true;

            m_parent->eofCallback(errno);
            return false;
        }
    }

    void
    TermClientProtocol::connSend(const char *buf, size_t len)
    {
        m_submachine->connSend(buf, len);
    }

    void
    TermClientProtocol::connFlush(const char *buf, size_t len)
    {
        m_submachine->connFlush(buf, len);
    }

    bool
    TermClientProtocol::protocolCallback(uint32_t command, uint32_t length, const char *body)
    {
        return m_parent->protocolCallback(command, length, body);
    }

    void
    TermClientProtocol::writeFd(const char *buf, size_t len)
    {
        char chunk[TERM_CHUNKSIZE];
        size_t rc;

        memcpy(chunk, TSQ_DATAPREFIX7, TSQ_DATAPREFIX7_LEN);
        memcpy(chunk + sizeof(chunk) - 2, TSQ_ST7, 2);

        while (len > TERM_PAYLOADSIZE) {
            ::base64(buf, TERM_PAYLOADSIZE, chunk + TSQ_DATAPREFIX7_LEN);
            len -= TERM_PAYLOADSIZE;
            buf += TERM_PAYLOADSIZE;

            m_parent->writeFd(chunk, sizeof(chunk));
        }
        if (len) {
            rc = ::base64(buf, len, chunk + TSQ_DATAPREFIX7_LEN);
            memcpy(chunk + rc + TSQ_DATAPREFIX7_LEN, TSQ_ST7, 2);

            m_parent->writeFd(chunk, rc + TSQ_DATAPREFIX7_LEN + 2);
        }
    }
}
