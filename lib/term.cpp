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
#include "utf8.h"

#include <unistd.h>

namespace Tsq
{
    inline void
    TermProtocol::init()
    {
        m_inbuf = new char[BODY_DEF_LENGTH];
        m_binbuf = new char[BODY_DEF_LENGTH];
        m_insize = BODY_DEF_LENGTH;
        m_inpos = 0;
        m_submachine = new RawProtocol(this);
        m_haveEsc = false;
        m_havePrefix = false;
        m_upos = 0;
    }

    TermProtocol::TermProtocol(ProtocolCallback *parent, const char *buf, size_t len) :
        ProtocolMachine(parent, buf, len)
    {
        init();
    }

    TermProtocol::TermProtocol(ProtocolCallback *parent) :
        ProtocolMachine(parent)
    {
        init();
    }

    TermProtocol::~TermProtocol()
    {
        delete [] m_inbuf;
    }

    bool
    TermProtocol::process(char c)
    {
        // UTF-8 parsing
        if (m_upos || c < 0) {
            m_ubuf[m_upos++] = c;

            try {
                uint32_t code = utf8::peek_next(m_ubuf, m_ubuf + m_upos);
                m_upos = 0;
                if (code > 255)
                    goto fail;
                c = (char)code;
            } catch (const utf8::not_enough_room &e) {
                return true;
            }
        }

        // Promote to 8-bit controls
        if (c == TSQ_ESC) {
            if (m_haveEsc)
                goto fail;
            m_haveEsc = true;
            return true;
        }
        if (m_haveEsc) {
            m_haveEsc = false;
            if (c == (char)(TSQ_OSC - 0x40))
                c = TSQ_OSC;
            else if (c == (char)(TSQ_ST - 0x40))
                c = TSQ_ST;
            else
                goto fail;
        }

        m_inbuf[m_inpos] = c;

        if (!m_havePrefix) {
            if (c != TSQ_DATAPREFIX[m_inpos])
                goto fail;
            if (++m_inpos == TSQ_DATAPREFIX_LEN) {
                m_havePrefix = true;
                m_inpos = 0;
            }
        }
        else if (c == TSQ_ST) {
            size_t rc = ::unbase64(m_inbuf, m_inpos, m_binbuf);
            m_havePrefix = false;
            m_inpos = 0;
            return m_submachine->connRead(m_binbuf, rc);
        }
        else if (!::is_base64_char(c) || ++m_inpos == m_insize)
            goto fail;

        return true;
    fail:
        // Ignore junk received between messages
        if (!m_havePrefix && m_inpos == 0)
            return true;

        throw ProtocolException(EPROTO);
    }

    bool
    TermProtocol::connRead(const char *buf, size_t len)
    {
        for (size_t i = 0; i < len; ++i)
            if (!process(buf[i]))
                return false;

        return true;
    }

    bool
    TermProtocol::connRead(int fd)
    {
        ssize_t got;
        char buf[READER_BUFSIZE];

        switch (got = read(fd, buf, sizeof(buf))) {
        default:
            for (size_t i = 0; i < got; ++i)
                if (!process(buf[i]))
                    return false;

            return true;
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
    TermProtocol::connSend(const char *buf, size_t len)
    {
        m_submachine->connSend(buf, len);
    }

    void
    TermProtocol::connFlush(const char *buf, size_t len)
    {
        m_submachine->connFlush(buf, len);
    }

    bool
    TermProtocol::protocolCallback(uint32_t command, uint32_t length, const char *body)
    {
        return m_parent->protocolCallback(command, length, body);
    }

    void
    TermProtocol::writeFd(const char *buf, size_t len)
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

    std::string
    TermProtocol::encode(const char *buf, size_t len)
    {
        char chunk[TERM_CHUNKSIZE];
        size_t rc;

        memcpy(chunk, TSQ_DATAPREFIX7, TSQ_DATAPREFIX7_LEN);

        if (len > TERM_PAYLOADSIZE)
            len = TERM_PAYLOADSIZE;

        rc = ::base64(buf, len, chunk + TSQ_DATAPREFIX7_LEN);
        memcpy(chunk + rc + TSQ_DATAPREFIX7_LEN, TSQ_ST7, 2);

        return std::string(chunk, rc + TSQ_DATAPREFIX7_LEN + 2);
    }

    void
    TermProtocol::reset()
    {
        m_submachine->reset();
        m_inpos = 0;
        m_haveEsc = false;
        m_havePrefix = false;
        m_upos = 0;
    }
}
