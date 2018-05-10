// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <functional>

namespace Tsq
{
    struct Uuid
    {
        const char buf[16];
        static const Uuid mt;

        inline Uuid(): buf{} {}
        Uuid(const char *bytes);
        Uuid(const std::string &str);
        Uuid(bool unused_generate);
        Uuid& operator=(const Uuid &o);

        inline bool operator==(const Uuid &o) const { return memcmp(buf, o.buf, 16) == 0; }
        inline bool operator!=(const Uuid &o) const { return memcmp(buf, o.buf, 16) != 0; }
        inline bool operator<(const Uuid &o) const { return memcmp(buf, o.buf, 16) < 0; }
        inline bool operator!() const { return memcmp(buf, mt.buf, 16) == 0; }
        inline explicit operator bool() const { return memcmp(buf, mt.buf, 16) != 0; }

        void generate();
        bool parse(const char *str);
        void combine(uint32_t mix);

        std::string str() const;
        std::string shortStr() const;
    };
}

namespace std
{
    template<>
    struct hash<Tsq::Uuid>
    {
        inline size_t operator()(const Tsq::Uuid &obj) const
        {
            if (sizeof(size_t) == 8)
                return *(reinterpret_cast<const uint64_t*>(obj.buf))  ^
                    *(reinterpret_cast<const uint64_t*>(obj.buf + 8));
            else
                return *(reinterpret_cast<const uint32_t*>(obj.buf))  ^
                    *(reinterpret_cast<const uint32_t*>(obj.buf + 4)) ^
                    *(reinterpret_cast<const uint32_t*>(obj.buf + 8)) ^
                    *(reinterpret_cast<const uint32_t*>(obj.buf + 12));
        }
    };

    template<>
    struct equal_to<Tsq::Uuid>
    {
        inline bool operator()(const Tsq::Uuid &lhs, const Tsq::Uuid &rhs) const
        {
            return memcmp(lhs.buf, rhs.buf, 16) == 0;
        }
    };
}

namespace Tsq
{
    // For use with Qt
    inline unsigned qHash(const Uuid &obj, unsigned seed)
    {
        return seed ^ *(reinterpret_cast<const uint32_t*>(obj.buf)) ^
            *(reinterpret_cast<const uint32_t*>(obj.buf + 4)) ^
            *(reinterpret_cast<const uint32_t*>(obj.buf + 8)) ^
            *(reinterpret_cast<const uint32_t*>(obj.buf + 12));
    }
}
