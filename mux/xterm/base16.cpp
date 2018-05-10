// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "base16.h"
#include "utf8.h"

static const char *const tab = "0123456789abcdef";

std::string
base16(const std::string &str)
{
    std::string result;
    for (unsigned char c: str) {
        result.push_back(tab[c >> 4]);
        result.push_back(tab[c & 0xf]);
    }
    return result;
}

static unsigned char
hexvalue(unsigned char d1, unsigned char d2)
{
    unsigned char result;

    if (d1 >= 'a')
        result = d1 - 'a' + 10;
    else if (d1 >= 'A')
        result = d1 - 'A' + 10;
    else
        result = d1 - '0';

    result <<= 4;

    if (d2 >= 'a')
        result |= d2 - 'a' + 10;
    else if (d2 >= 'A')
        result |= d2 - 'A' + 10;
    else
        result |= d2 - '0';

    return result;
}

bool
unbase16_inplace_utf8(std::string &str)
{
    size_t n = str.size();

    if (strspn(str.data(), "0123456789abcdefABCDEF") != n)
        return false;

    for (size_t i = 0; i < n; i += 2)
        str[i / 2] = hexvalue(str[i], str[i + 1]);

    str.resize(n / 2);
    return utf8::is_valid(str.cbegin(), str.cend()) &&
        str.find('\0') == std::string::npos;
}
