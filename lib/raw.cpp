// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "raw.h"
#include "endian.h"
#include "config.h"
#include "exception.h"

#include <unistd.h>

// can't exceed BODY_DEF_LENGTH
#define HEADER_SIZE 8

static const char padding[4] = { 0 };

namespace Tsq
{
    inline void
    RawProtocol::init()
    {
        m_inbuf = new char[BODY_DEF_LENGTH];
        m_outbuf = new char[WRITER_BUFSIZE];
        m_insize = BODY_DEF_LENGTH;
        m_inpos = 0;
        m_outsize = WRITER_BUFSIZE;
        m_outpos = 0;
        m_haveHeader = false;
        m_haveBody = false;
    }

    RawProtocol::RawProtocol(ProtocolCallback *parent, const char *buf, size_t len) :
        ProtocolMachine(parent, buf, len)
    {
        init();
    }

    RawProtocol::RawProtocol(ProtocolCallback *parent) :
        ProtocolMachine(parent)
    {
        init();
    }

    RawProtocol::~RawProtocol()
    {
        delete [] m_outbuf;
        delete [] m_inbuf;
    }

    int
    RawProtocol::connProcess(const char *buf, size_t len)
    {
        const char *ptr = buf;
        unsigned i = 0;
        bool rc;

        if (m_haveHeader)
            goto haveHeader;

        for (; i < len; ++i)
        {
            m_inbuf[m_inpos++] = *ptr++;
            if (m_inpos == HEADER_SIZE)
            {
                m_command = le32toh(*((uint32_t *)m_inbuf));
                m_payloadlength = le32toh(*((uint32_t *)(m_inbuf + 4)));

                /* pad payload length out to 4 bytes */
                m_length = ((m_payloadlength + 3) / 4) * 4;

                if (m_length > BODY_MAX_LENGTH) {
                    throw ProtocolException(EMSGSIZE);
                }
                if (m_length > m_insize) {
                    do {
                        m_insize *= 2;
                    } while (m_length > m_insize);

                    delete [] m_inbuf;
                    m_inbuf = new char[m_insize];
                }

                len -= ++i;
                m_inpos = 0;
                m_haveHeader = true;
                goto haveHeader;
            }
        }
        return i;

    haveHeader:
        // do we have the body all right here?
        // if so, don't bother copying it
        if (m_inpos == 0 && m_length <= len) {
            m_haveHeader = false;
            rc = m_parent->protocolCallback(m_command, m_payloadlength, ptr);
            return rc ? i + m_length : -1;
        }

        // can we complete the body?
        if (m_length - m_inpos <= len) {
            i = m_length - m_inpos;
            memcpy(m_inbuf + m_inpos, ptr, i);
            m_inpos = 0;
            m_haveHeader = false;
            rc = m_parent->protocolCallback(m_command, m_payloadlength, m_inbuf);
            return rc ? i : -1;
        }

        memcpy(m_inbuf + m_inpos, ptr, len);
        m_inpos += len;
        return i + len;
    }

    bool
    RawProtocol::connRead(const char *buf, size_t len)
    {
        size_t done = 0;

        // feed the machine, repeatedly
        while (done < len) {
            int rc = connProcess(buf + done, len - done);
            if (rc == -1)
                return false;
            done += rc;
        }
        return true;
    }

    bool
    RawProtocol::connRead(int fd)
    {
        ssize_t got;
        size_t done;
        char buf[READER_BUFSIZE];

        switch (got = read(fd, buf, sizeof(buf))) {
        default:
            // feed the machine, repeatedly
            done = 0;
            while (done < got) {
                int rc = connProcess(buf + done, got - done);
                if (rc == -1)
                    return false;
                done += rc;
            }
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
    RawProtocol::connSend(const char *buf, size_t len)
    {
        size_t room;
        unsigned padoff = len & 3;

    restart:
        while (1) {
            room = m_outsize - m_outpos;

            if (room == 0) {
                m_parent->writeFd(m_outbuf, m_outsize);
                m_outpos = 0;
            }
            else if (room >= len) {
                memcpy(m_outbuf + m_outpos, buf, len);
                m_outpos += len;
                break;
            }
            else {
                memcpy(m_outbuf + m_outpos, buf, room);
                m_outpos += room;
                buf += room;
                len -= room;
            }
        }

        /* pad length out to 4 bytes */
        if (padoff) {
            buf = padding;
            len = 4 - padoff;
            padoff = 0;
            goto restart;
        }
    }

    void
    RawProtocol::connFlush(const char *buf, size_t len)
    {
        if (m_outpos) {
            m_parent->writeFd(m_outbuf, m_outpos);
            m_outpos = 0;
        }
        if (len) {
            m_parent->writeFd(buf, len);

            unsigned padoff = len & 3;
            if (padoff) {
                m_parent->writeFd(padding, 4 - padoff);
            }
        }
    }

    void
    RawProtocol::reset()
    {
        m_inpos = 0;
        m_outpos = 0;
        m_haveHeader = false;
        m_haveBody = false;
    }
}
