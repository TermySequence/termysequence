// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <vector>

namespace Tsq
{
    enum HandshakeResult {
        ShakeOngoing = 0,
        ShakeSuccess = 1,
        ShakeBadLeadingContent  = -1,
        ShakeBadEscapeSequence  = -2,
        ShakeBadPrefix          = -3,
        ShakeBadServerVersion   = -4,
        ShakeBadClientVersion   = -4,
        ShakeBadProtocolVersion = -5,
        ShakeBadProtocolType    = -5,
        ShakeBadServerUuid      = -6,
        ShakeBadClientUuid      = -6,
        ShakeTooLong            = -7,
        ShakeBadUtf8            = -8,
    };

    class ClientHandshake
    {
    private:
        const char *const id;

        char ubuf[8];
        size_t upos;

        std::string buf;
        std::vector<uint32_t> scratch;

        bool haveEsc;
        bool havePrefix;
        bool haveServerVersion;
        bool haveProtocolVersion;

        bool strict;
        bool haveCsi;
        bool haveOsc;

        int processStrictByte(char c);
        int processScratchByte(char c);

        int processStrict(const char *buf, size_t len);

    public:
        ClientHandshake(const char *clientId, bool strict);
        /* Reset state */
        void reset(bool strict);

        /* Read hello message from server */
        int processHelloByte(char c);
        /* Read hello message from server */
        int processHello(const char *buf, size_t len);
        /* Server version */
        unsigned serverVersion;
        /* Protocol version */
        unsigned protocolVersion;
        /* Server UUID */
        char serverId[16];
        /* Pointer to any leftover bytes in last buf */
        const char *residualBuf;
        /* Length of remaining bytes in last buf */
        size_t residualLen;
        /* Pointer to any invalid leading content */
        const char *leadingContent;
        /* Length of invalid leading content */
        size_t leadingContentLen;

        /* Get response bytes to send */
        std::string getResponse(unsigned clientVersion, unsigned protocolType);
        /* Get copy of server hello */
        std::string getHello();
    };

    class ServerHandshake
    {
    private:
        const char *const id;

        char ubuf[8];
        size_t upos;

        char *buf;
        size_t pos;

        bool haveEsc;
        bool havePrefix;
        bool haveClientVersion;
        bool haveProtocolType;

    public:
        ServerHandshake(const char *serverId);
        ~ServerHandshake();
        /* Reset state */
        void reset();

        /* Get hello bytes to send */
        std::string getHello(unsigned serverVersion, unsigned protocolVersion);

        /* Read response message from client */
        int processResponseByte(char c);
        /* Read response message from client */
        int processResponse(const char *buf, size_t len);
        /* Client version */
        unsigned clientVersion;
        /* Requested protocol type */
        unsigned protocolType;
        /* Client UUID */
        char clientId[16];
        /* Pointer to any leftover bytes in last buf */
        const char *residualBuf;
        /* Length of remaining bytes in last buf */
        size_t residualLen;

        /* Get copy of client response */
        std::string getResponse();
    };
}
