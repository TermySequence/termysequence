// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "uuid.h"
#include "endian.h"

namespace Tsq
{
    class ProtocolMarshaler
    {
    private:
        std::string m_buf;

    public:
        ProtocolMarshaler();
        ProtocolMarshaler(uint32_t command);
        ProtocolMarshaler(uint32_t command, uint32_t length, const char *body);
        void begin(uint32_t command);
        void begin(uint32_t command, uint32_t length, const char *body);
        void setCommand(uint32_t command);

        inline uint32_t length() const { return m_buf.size(); }
        inline std::string& result() { return m_buf; }
        inline const char *resultPtr() const { return m_buf.data(); }

        void addUuid(const char *uuid);
        void addUuid(const Uuid &uuid);
        void addUuidPair(const Uuid &uuid1, const Uuid &uuid2);
        void addUuidPairReversed(const char *buf);
        void addNumber(uint32_t num);
        void addNumberPair(uint32_t num1, uint32_t num2);
        void addNumber64(uint64_t num);

        void addBytes(const std::string &str);
        void addBytes(const char *buf, uint32_t len);
        void addString(const std::string &str);
        void addString(const char *str);
        void addString(const char *buf, uint32_t len);
        void addStringPair(const std::string &key, const std::string &value);
        void addStringPair(const char *key, const char *value);

        void addPaddedString(const std::string &str);
        void addPadding();
    };

    class ProtocolUnmarshaler
    {
    private:
        const char *m_buf;
        uint32_t m_len, m_pos;

    public:
        ProtocolUnmarshaler();
        ProtocolUnmarshaler(const char *body, uint32_t len);
        void begin(const char *body, uint32_t len);

        inline uint32_t currentPosition() const { return m_pos; }
        inline uint32_t remainingLength() const { return m_len - m_pos; }
        inline const char* remainingBytes() const { return m_buf + m_pos; }

        const Uuid parseUuid();
        uint32_t parseNumber();
        std::pair<uint32_t,uint32_t> parseNumberPair();
        uint64_t parseNumber64();
        uint32_t parseOptionalNumber(uint32_t defval = 0);

        const char* parseBytes(uint32_t len);
        std::string parseString();
        const char* parseString(uint32_t *lenret);
        const char* parsePaddedString();

        void validateAsUtf8();
        std::string parseUtf8();
    };
}
