// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "handshake.h"
#include "sequences.h"
#include "utf8.h"

#include <cstdio>
#include <uuid/uuid.h>

#define HANDSHAKE_BUFSIZE 128
#define MAX_NUMBER_WIDTH 4
#define UUID_SIZE 36

namespace Tsq
{
    int
    ServerHandshake::processResponseByte(char c)
    {
        // UTF-8 parsing
        ubuf[upos++] = c;

        try {
            uint32_t code = utf8::peek_next(ubuf, ubuf + upos);
            if (code > 255)
                return ShakeBadUtf8;
            upos = 0;
            c = (char)code;
        } catch (const utf8::not_enough_room &e) {
            return ShakeOngoing;
        } catch (const utf8::exception &e) {
            return ShakeBadUtf8;
        }

        // Promote to 8-bit controls
        if (c == TSQ_ESC) {
            if (haveEsc)
                return ShakeBadEscapeSequence;
            haveEsc = true;
            return ShakeOngoing;
        }
        if (haveEsc) {
            haveEsc = false;
            if (c == (char)(TSQ_OSC - 0x40))
                c = TSQ_OSC;
            else if (c == (char)(TSQ_ST - 0x40))
                c = TSQ_ST;
            else
                return ShakeBadEscapeSequence;
        }

        buf[pos++] = c;

        if (!havePrefix) {
            if (c != TSQ_HANDSHAKE[pos - 1])
                return ShakeBadPrefix;
            if (pos == TSQ_HANDSHAKE_LEN) {
                havePrefix = true;
                pos = 0;
            }
        }
        else if (!haveClientVersion) {
            if (c == ';') {
                buf[pos - 1] = '\0';
                if (strspn(buf, "0123456789") != pos - 1)
                    return ShakeBadClientVersion;
                clientVersion = atoi(buf);
                if (clientVersion == 0)
                    return ShakeBadClientVersion;
                haveClientVersion = true;
                pos = 0;
            }
            else if (pos > MAX_NUMBER_WIDTH)
                return ShakeBadClientVersion;
        }
        else if (!haveProtocolType) {
            if (c == ';') {
                buf[pos - 1] = '\0';
                if (strspn(buf, "0123456789") != pos - 1)
                    return ShakeBadProtocolType;
                protocolType = atoi(buf);
                haveProtocolType = true;
                pos = 0;
            }
            else if (pos > MAX_NUMBER_WIDTH)
                return ShakeBadProtocolType;
        }
        else {
            if (c == TSQ_ST) {
                buf[pos - 1] = '\0';
                if (pos - 1 != UUID_SIZE)
                    return ShakeBadClientUuid;
                if (strspn(buf, "0123456789abcdefABCDEF-") != UUID_SIZE)
                    return ShakeBadClientUuid;
                if (uuid_parse(buf, (unsigned char *)clientId) < 0)
                    return ShakeBadClientUuid;
                // done at this point
                return ShakeSuccess;
            }
            else if (pos > UUID_SIZE)
                return ShakeBadClientUuid;
        }

        return ShakeOngoing;
    }

    void
    ServerHandshake::reset()
    {
        upos = 0;
        pos = 0;
        haveEsc = false;
        havePrefix = false;
        haveClientVersion = false;
        haveProtocolType = false;
    }

    ServerHandshake::ServerHandshake(const char *serverId) :
        id(serverId)
    {
        buf = new char[HANDSHAKE_BUFSIZE];
        reset();
    }

    ServerHandshake::~ServerHandshake()
    {
        delete [] buf;
    }

    std::string
    ServerHandshake::getHello(unsigned serverVersion, unsigned protocolVersion)
    {
        char buf[HANDSHAKE_BUFSIZE];
        char subbuf[37];

        uuid_unparse((const unsigned char *)id, subbuf);

        sprintf(buf, TSQ_HANDSHAKE7 "%u;%u;%s" TSQ_ST7,
                serverVersion, protocolVersion, subbuf);
        return std::string(buf);
    }

    int
    ServerHandshake::processResponse(const char *buf, size_t len)
    {
        for (size_t i = 0; i < len;)
        {
            int rc = processResponseByte(buf[i++]);
            switch (rc) {
            case ShakeOngoing:
                continue;
            case ShakeSuccess:
                residualBuf = buf + i;
                residualLen = len - i;
                // fallthru
            default:
                return rc;
            }
        }
        return ShakeOngoing;
    }

    std::string
    ServerHandshake::getResponse()
    {
        char buf[HANDSHAKE_BUFSIZE];
        char subbuf[37];

        uuid_unparse((const unsigned char *)clientId, subbuf);

        sprintf(buf, TSQ_HANDSHAKE7 "%u;%u;%s" TSQ_ST7,
                clientVersion, protocolType, subbuf);
        return std::string(buf);
    }
}
