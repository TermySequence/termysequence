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
#define SCRATCH_MAX 4000
#define MAX_NUMBER_WIDTH 4
#define UUID_SIZE 36

namespace Tsq
{
    int
    ClientHandshake::processStrictByte(char c)
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

        buf.push_back(c);
        unsigned pos = buf.size();

        if (!havePrefix) {
            if (c != TSQ_HANDSHAKE[pos - 1])
                return ShakeBadPrefix;
            if (pos == TSQ_HANDSHAKE_LEN) {
                havePrefix = true;
                buf.clear();
            }
        }
        else if (!haveServerVersion) {
            if (c == ';') {
                buf[pos - 1] = '\0';
                if (strspn(buf.data(), "0123456789") != pos - 1)
                    return ShakeBadServerVersion;
                serverVersion = atoi(buf.data());
                if (serverVersion == 0)
                    return ShakeBadServerVersion;
                haveServerVersion = true;
                buf.clear();
            }
            else if (pos > MAX_NUMBER_WIDTH)
                return ShakeBadServerVersion;
        }
        else if (!haveProtocolVersion) {
            if (c == ';') {
                buf[pos - 1] = '\0';
                if (strspn(buf.data(), "0123456789") != pos - 1)
                    return ShakeBadProtocolVersion;
                protocolVersion = atoi(buf.data());
                if (protocolVersion == 0)
                    return ShakeBadProtocolVersion;
                haveProtocolVersion = true;
                buf.clear();
            }
            else if (pos > MAX_NUMBER_WIDTH)
                return ShakeBadProtocolVersion;
        }
        else {
            if (c == TSQ_ST) {
                buf[pos - 1] = '\0';
                if (pos - 1 != UUID_SIZE)
                    return ShakeBadServerUuid;
                if (strspn(buf.data(), "0123456789abcdefABCDEF-") != UUID_SIZE)
                    return ShakeBadServerUuid;
                if (uuid_parse(buf.data(), (unsigned char *)serverId) < 0)
                    return ShakeBadServerUuid;
                // done at this point
                return ShakeSuccess;
            }
            else if (pos > UUID_SIZE)
                return ShakeBadServerUuid;
        }

        return ShakeOngoing;
    }

    /*
     * A very basic state machine for detecting CSI and OSC sequences.
     * The goal is to simply skip past spurious sequences printed by user
     * login scripts, while letting through printable characters, before
     * ultimately reaching the handshake sequence.
     */
    static const char *const s_cterms = "@ABCDEFGHIJKLMPSTXZ`abcdefghilmnpqrstuvwxyz{|}~";
    static const char *const s_oterms = "\x07\x9c";

    static inline bool
    codein(const char *str, uint32_t code)
    {
        const unsigned char *ptr;
        for (ptr = (const unsigned char*)str; *ptr && *ptr != code; ++ptr);
        return *ptr != 0;
    }

    static inline bool
    codecmp(const uint32_t *buf, const char *str, unsigned len)
    {
        unsigned i;
        const unsigned char *ustr = (const unsigned char*)str;
        for (i = 0; i < len && buf[i] == ustr[i]; ++i);
        return i == len;
    }

    int
    ClientHandshake::processScratchByte(char c)
    {
        // UTF-8 parsing
        uint32_t code;
        ubuf[upos++] = c;
        leadingContentLen = upos;

        try {
            code = utf8::peek_next(ubuf, ubuf + upos);
            upos = 0;
        } catch (const utf8::not_enough_room &e) {
            return ShakeOngoing;
        } catch (const utf8::exception &e) {
            return ShakeBadUtf8;
        }

        // Promote to 8-bit controls
        if (code == (unsigned char)TSQ_ESC) {
            if (haveEsc)
                return ShakeBadEscapeSequence;
            haveEsc = true;
            return ShakeOngoing;
        }
        if (haveEsc) {
            haveEsc = false;
            if (code >= 0x40 && code <= 0x5f)
                code += 0x40;
            // otherwise just ignore the Esc
        }

        if (haveCsi) {
            if (codein(s_cterms, code))
                haveCsi = false;
            return ShakeOngoing;
        }
        if (haveOsc) {
            scratch.push_back(code);

            if (scratch.size() > SCRATCH_MAX)
                return ShakeTooLong;
            if (!codein(s_oterms, code))
                return ShakeOngoing;

            haveOsc = false;

            if (scratch.size() < TSQ_HANDSHAKE_LEN ||
                !codecmp(scratch.data(), TSQ_HANDSHAKE, TSQ_HANDSHAKE_LEN)) {
                scratch.clear();
                return ShakeOngoing;
            }

            std::string tmp;
            utf8::unchecked::utf32to8(scratch.begin(), scratch.end(),
                                      std::back_inserter(tmp));
            return processStrict(tmp.data(), tmp.size());
        }
        if (code == (unsigned char)TSQ_OSC) {
            scratch.push_back(code);
            haveOsc = true;
            return ShakeOngoing;
        }
        if (code == (unsigned char)TSQ_CSI) {
            haveCsi = true;
            return ShakeOngoing;
        }

        return ShakeBadLeadingContent;
    }

    int
    ClientHandshake::processHelloByte(char c)
    {
        return strict ? processStrictByte(c) : processScratchByte(c);
    }

    void
    ClientHandshake::reset(bool strict_)
    {
        upos = 0;
        buf.clear();
        scratch.clear();

        haveEsc = false;
        havePrefix = false;
        haveServerVersion = false;
        haveProtocolVersion = false;
        strict = strict_;
        haveCsi = false;
        haveOsc = false;
    }

    ClientHandshake::ClientHandshake(const char *clientId, bool strict_) :
        id(clientId)
    {
        leadingContent = ubuf;
        reset(strict_);
    }

    int
    ClientHandshake::processStrict(const char *buf, size_t len)
    {
        for (size_t i = 0; i < len;)
        {
            int rc = processStrictByte(buf[i++]);
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

    int
    ClientHandshake::processHello(const char *buf, size_t len)
    {
        if (strict)
            return processStrict(buf, len);

        for (size_t i = 0; i < len;)
        {
            int rc = processScratchByte(buf[i++]);
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
    ClientHandshake::getResponse(unsigned clientVersion, unsigned protocolType)
    {
        char buf[HANDSHAKE_BUFSIZE];
        char subbuf[37];

        uuid_unparse((const unsigned char *)id, subbuf);

        sprintf(buf, TSQ_HANDSHAKE7 "%u;%u;%s" TSQ_ST7,
                clientVersion, protocolType, subbuf);
        return std::string(buf);
    }

    std::string
    ClientHandshake::getHello()
    {
        char buf[HANDSHAKE_BUFSIZE];
        char subbuf[37];

        uuid_unparse((const unsigned char *)serverId, subbuf);

        sprintf(buf, TSQ_HANDSHAKE7 "%u;%u;%s" TSQ_ST7,
                serverVersion, protocolVersion, subbuf);
        return std::string(buf);
    }
}
