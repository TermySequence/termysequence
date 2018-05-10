// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "uuid.h"
#include "base64.h"

#include <uuid/uuid.h>
#include <random>

namespace Tsq
{
    const Uuid Uuid::mt;

    Uuid::Uuid(const char *bytes) : buf{}
    {
        memcpy(const_cast<char*>(buf), bytes, 16);
    }

    Uuid::Uuid(const std::string &str) : buf{}
    {
        parse(str.c_str());
    }

    Uuid::Uuid(bool) : buf{}
    {
        uuid_generate((unsigned char*)buf);
    }

    void
    Uuid::generate()
    {
        uuid_generate((unsigned char*)buf);
    }

    bool
    Uuid::parse(const char *str)
    {
        char tmp[37];

        if (strlen(str) == 32) {
            // insert dashes...
            memcpy(tmp, str, 8);
            tmp[8] = '-';
            memcpy(tmp + 9, str + 8, 4);
            tmp[13] = '-';
            memcpy(tmp + 14, str + 12, 4);
            tmp[18] = '-';
            memcpy(tmp + 19, str + 16, 4);
            tmp[23] = '-';
            memcpy(tmp + 24, str + 20, 12);
            tmp[36] = '\0';

            str = tmp;
        }
        return uuid_parse(str, (unsigned char *)buf) == 0;
    }

    void
    Uuid::combine(uint32_t mix)
    {
        if (mix) {
            std::minstd_rand gen(mix);
            gen.discard(mix % 512);
            mix = gen();
            auto *bytes = (const unsigned char*)&mix;
            char *ptr = const_cast<char*>(buf);

            for (int i = 0; i < 16; ++i)
                ptr[i] ^= bytes[i & 3];

            ptr[6] = (ptr[6] & 0x0f) | 0x40;
            ptr[8] = (ptr[8] & 0x3f) | 0x80;
        }
    }

    Uuid&
    Uuid::operator=(const Tsq::Uuid &o)
    {
        memcpy(const_cast<char*>(buf), o.buf, 16);
        return *this;
    }

    std::string
    Uuid::str() const
    {
        char tmp[37];
        uuid_unparse_lower((const unsigned char*)buf, tmp);
        return std::string(tmp);
    }

    std::string
    Uuid::shortStr() const
    {
        // 84 byte hash, base64-encoded, [+/] squashed
        char hash[11];
        memcpy(hash, buf, sizeof(hash));
        for (int i = 4; i < 10; ++i)
            hash[i] ^= buf[i + 6];

        char dst[15];
        base64(hash, sizeof(hash), dst);
        for (int i = 0; i < 14; ++i)
            if (dst[i] == '/' || dst[i] == '+')
                dst[i] = 'a';
        return std::string(dst, 14);
    }
}
