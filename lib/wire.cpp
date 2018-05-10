// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "wire.h"
#include "exception.h"
#include "utf8.h"

namespace Tsq
{
    /*
     * Raw Marshaler
     */
    ProtocolMarshaler::ProtocolMarshaler()
    {
        m_buf.append(8, '\0');
    }

    ProtocolMarshaler::ProtocolMarshaler(uint32_t command)
    {
        uint32_t tmp = htole32(command);
        m_buf.append(reinterpret_cast<char*>(&tmp), 4);
        m_buf.append(4, '\0');
    }

    ProtocolMarshaler::ProtocolMarshaler(uint32_t command, uint32_t length, const char *body)
    {
        uint32_t tmp = htole32(command);
        m_buf.append(reinterpret_cast<char*>(&tmp), 4);
        tmp = htole32(length);
        m_buf.append(reinterpret_cast<char*>(&tmp), 4);
        m_buf.append(body, length);
    }

    void
    ProtocolMarshaler::begin(uint32_t command)
    {
        m_buf.clear();
        uint32_t tmp = htole32(command);
        m_buf.append(reinterpret_cast<char*>(&tmp), 4);
        m_buf.append(4, '\0');
    }

    void
    ProtocolMarshaler::begin(uint32_t command, uint32_t length, const char *body)
    {
        m_buf.clear();
        uint32_t tmp = htole32(command);
        m_buf.append(reinterpret_cast<char*>(&tmp), 4);
        tmp = htole32(length);
        m_buf.append(reinterpret_cast<char*>(&tmp), 4);
        m_buf.append(body, length);
    }

    void
    ProtocolMarshaler::setCommand(uint32_t command)
    {
        uint32_t tmp = htole32(command);
        m_buf.replace(0, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addUuid(const char *uuid)
    {
        m_buf.append(uuid, 16);
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addUuid(const Uuid &uuid)
    {
        m_buf.append(uuid.buf, 16);
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addUuidPair(const Uuid &uuid1, const Uuid &uuid2)
    {
        m_buf.append(uuid1.buf, 16);
        m_buf.append(uuid2.buf, 16);
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addUuidPairReversed(const char *buf)
    {
        m_buf.append(buf + 16, 16);
        m_buf.append(buf, 16);
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addNumber(uint32_t num)
    {
        num = htole32(num);
        m_buf.append(reinterpret_cast<char*>(&num), 4);
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addNumberPair(uint32_t num1, uint32_t num2)
    {
        num1 = htole32(num1);
        num2 = htole32(num2);
        m_buf.append(reinterpret_cast<char*>(&num1), 4);
        m_buf.append(reinterpret_cast<char*>(&num2), 4);
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addNumber64(uint64_t num)
    {
        num = htole64(num);
        m_buf.append(reinterpret_cast<char*>(&num), 8);
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }


    void
    ProtocolMarshaler::addBytes(const std::string &str)
    {
        m_buf.append(str);
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addBytes(const char *buf, uint32_t len)
    {
        m_buf.append(buf, len);
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addString(const std::string &str)
    {
        m_buf.append(str);
        m_buf.push_back('\0');
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addString(const char *str)
    {
        m_buf.append(str);
        m_buf.push_back('\0');
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addString(const char *buf, uint32_t len)
    {
        m_buf.append(buf, len);
        m_buf.push_back('\0');
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addStringPair(const std::string &key, const std::string &value)
    {
        m_buf.append(key);
        m_buf.push_back('\0');
        m_buf.append(value);
        m_buf.push_back('\0');
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addStringPair(const char *key, const char *value)
    {
        m_buf.append(key);
        m_buf.push_back('\0');
        m_buf.append(value);
        m_buf.push_back('\0');
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addPaddedString(const std::string &str)
    {
        m_buf.append(str);
        m_buf.append(4 - (str.size() & 3), '\0');
        uint32_t tmp = htole32(m_buf.size() - 8);
        m_buf.replace(4, 4, reinterpret_cast<char*>(&tmp), 4);
    }

    void
    ProtocolMarshaler::addPadding()
    {
        unsigned pad;

        if ((pad = m_buf.size() & 3))
            m_buf.append(4 - pad, '\0');
    }

    /*
     * Raw Unmarshaler
     */
    ProtocolUnmarshaler::ProtocolUnmarshaler()
    {
        m_buf = "";
        m_len = m_pos = 0;
    }

    ProtocolUnmarshaler::ProtocolUnmarshaler(const char *body, uint32_t len)
    {
        m_buf = body;
        m_len = len;
        m_pos = 0;
    }

    void
    ProtocolUnmarshaler::begin(const char *body, uint32_t len)
    {
        m_buf = body;
        m_len = len;
        m_pos = 0;
    }


    const Uuid
    ProtocolUnmarshaler::parseUuid()
    {
        const char *ptr = m_buf + m_pos;
        m_pos += 16;
        if (m_pos > m_len)
            throw ProtocolException();
        return Uuid(ptr);
    }

    uint32_t
    ProtocolUnmarshaler::parseNumber()
    {
        const char *ptr = m_buf + m_pos;
        m_pos += 4;
        if (m_pos > m_len)
            throw ProtocolException();
        return le32toh(*(reinterpret_cast<const uint32_t*>(ptr)));
    }

    std::pair<uint32_t,uint32_t>
    ProtocolUnmarshaler::parseNumberPair()
    {
        const char *ptr = m_buf + m_pos;
        m_pos += 8;
        if (m_pos > m_len)
            throw ProtocolException();

        return std::pair<uint32_t,uint32_t>(
            le32toh(*(reinterpret_cast<const uint32_t*>(ptr))),
            le32toh(*(reinterpret_cast<const uint32_t*>(ptr + 4)))
            );
    }

    uint64_t
    ProtocolUnmarshaler::parseNumber64()
    {
        const char *ptr = m_buf + m_pos;
        m_pos += 8;
        if (m_pos > m_len)
            throw ProtocolException();

        uint64_t lower = le32toh(*(reinterpret_cast<const uint32_t*>(ptr)));
        uint64_t upper = le32toh(*(reinterpret_cast<const uint32_t*>(ptr + 4)));
        return (upper << 32)|lower;
    }

    uint32_t
    ProtocolUnmarshaler::parseOptionalNumber(uint32_t defval)
    {
        if (m_pos + 4 > m_len)
            return defval;

        const char *ptr = m_buf + m_pos;
        m_pos += 4;
        return le32toh(*(reinterpret_cast<const uint32_t*>(ptr)));
    }


    const char *
    ProtocolUnmarshaler::parseBytes(uint32_t len)
    {
        const char *ptr = m_buf + m_pos;
        m_pos += len;
        if (m_pos > m_len)
            throw ProtocolException();

        return ptr;
    }

    std::string
    ProtocolUnmarshaler::parseString()
    {
        uint32_t i;
        // find end of string or nul, whichever comes first
        for (i = m_pos; i < m_len && m_buf[i]; ++i);
        std::string retval(m_buf + m_pos, i - m_pos);
        m_pos = (i < m_len) ? i + 1 : i;
        return retval;
    }

    const char *
    ProtocolUnmarshaler::parseString(uint32_t *lenret)
    {
        uint32_t i;
        // find end of string or nul, whichever comes first
        for (i = m_pos; i < m_len && m_buf[i]; ++i);
        const char *retval = m_buf + m_pos;
        *lenret = i - m_pos;
        m_pos = (i < m_len) ? i + 1 : i;
        return retval;
    }

    const char *
    ProtocolUnmarshaler::parsePaddedString()
    {
        uint32_t save, i;

        // find nul
        for (save = i = m_pos; i < m_len && m_buf[i]; ++i);
        if (i == m_len)
            throw ProtocolException();

        // make sure padding is correct
        int pad = 3 - (i & 3);
        while (pad--) {
            ++i;
            if (i == m_len || m_buf[i])
                throw ProtocolException();
        }

        m_pos = i + 1;
        return m_buf + save;
    }

    void
    ProtocolUnmarshaler::validateAsUtf8()
    {
        if (!utf8::is_valid(m_buf + m_pos, m_buf + m_len))
            throw ProtocolException();
    }

    std::string
    ProtocolUnmarshaler::parseUtf8()
    {
        std::string result = parseString();

        if (!utf8::is_valid(result.cbegin(), result.cend()))
            throw ProtocolException();

        return result;
    }
}
