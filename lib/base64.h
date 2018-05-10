// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

static inline bool
is_base64_char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '+' || c == '/';
}

static inline bool
is_base64_str(const char *buf, size_t buflen)
{
    for (size_t i = 0; i < buflen; ++i)
        if (!is_base64_char(buf[i]))
            return false;

    return true;
}

// Does not add padding
extern size_t
base64(const char *buf, size_t buflen, char *dst);

// Does add padding
extern void
base64(const char *buf, size_t buflen, std::string &dst);

//
// Unchecked, no padding or whitespace
//
extern size_t
unbase64(const char *buf, size_t buflen, char *dst);

//
// Permits whitespace and padding
//
extern bool
unbase64_validate(const std::string &str);

extern bool
unbase64_inplace(std::string &str);

extern bool
unbase64_inplace_utf8(std::string &str);

extern bool
unbase64_inplace_hash(std::string &str, size_t offset, uint64_t &hashret);
