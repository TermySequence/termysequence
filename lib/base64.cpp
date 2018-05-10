// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "base64.h"
#include "utf8.h"

static const char *const tab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char *const whitespace = " \r\n\t\v";

size_t
base64(const char *buf, size_t buflen, char *dst)
{
    const auto *ubuf = reinterpret_cast<const unsigned char*>(buf);
    size_t i, n;
    unsigned int cur;

    i = 0;
    n = (buflen / 3) * 4;

    while (i < n) {
        cur = (ubuf[0] << 16) | (ubuf[1] << 8) | ubuf[2];

        dst[i++] = tab[(cur >> 18)];
        dst[i++] = tab[(cur >> 12) & 0x3f];
        dst[i++] = tab[(cur >> 6) & 0x3f];
        dst[i++] = tab[cur & 0x3f];

        ubuf += 3;
    }

    switch (buflen % 3) {
    case 2:
        cur = (ubuf[0] << 16) | (ubuf[1] << 8);

        dst[i++] = tab[(cur >> 18)];
        dst[i++] = tab[(cur >> 12) & 0x3f];
        dst[i++] = tab[(cur >> 6) & 0x3f];
        break;
    case 1:
        cur = (ubuf[0] << 16);

        dst[i++] = tab[(cur >> 18)];
        dst[i++] = tab[(cur >> 12) & 0x3f];
        break;
    }

    return i;
}

void
base64(const char *buf, size_t buflen, std::string &dst)
{
    const auto *ubuf = reinterpret_cast<const unsigned char*>(buf);
    size_t i, n;
    unsigned int cur;

    i = 0;
    n = (buflen / 3) * 4;
    dst.reserve(n);

    while (i < n) {
        cur = (ubuf[0] << 16) | (ubuf[1] << 8) | ubuf[2];

        dst.push_back(tab[(cur >> 18)]);
        dst.push_back(tab[(cur >> 12) & 0x3f]);
        dst.push_back(tab[(cur >> 6) & 0x3f]);
        dst.push_back(tab[cur & 0x3f]);

        i += 4;
        ubuf += 3;
    }

    switch (buflen % 3) {
    case 2:
        cur = (ubuf[0] << 16) | (ubuf[1] << 8);

        dst.push_back(tab[(cur >> 18)]);
        dst.push_back(tab[(cur >> 12) & 0x3f]);
        dst.push_back(tab[(cur >> 6) & 0x3f]);
        dst.push_back('=');
        break;
    case 1:
        cur = (ubuf[0] << 16);

        dst.push_back(tab[(cur >> 18)]);
        dst.push_back(tab[(cur >> 12) & 0x3f]);
        dst.append(2, '=');
        break;
    }
}

size_t
unbase64(const char *buf, size_t buflen, char *dst)
{
    size_t i = 0, n = 0, rc = 0;
    unsigned int cur;
    char a, b;

    if (buflen) {
        while (n + 4 < buflen) {
            cur = ((strchr(tab, buf[0]) - tab) << 18) |
                ((strchr(tab, buf[1]) - tab) << 12) |
                ((strchr(tab, buf[2]) - tab) << 6) |
                (strchr(tab, buf[3]) - tab);

            dst[i++] = cur >> 16;
            dst[i++] = (cur >> 8) & 0xff;
            dst[i++] = cur & 0xff;

            buf += 4;
            n += 4;
        }

        switch (buflen - n) {
        case 4:
            a = buf[2];
            b = buf[3];
            rc = i + 3;
            break;
        case 3:
            a = buf[2];
            b = 0;
            rc = i + 2;
            break;
        case 2:
            a = 0;
            b = 0;
            rc = i + 1;
            break;
        default:
            goto out;
        }

        cur = ((strchr(tab, buf[0]) - tab) << 18) |
            ((strchr(tab, buf[1]) - tab) << 12) |
            ((strchr(tab, a) - tab) << 6) |
            (strchr(tab, b) - tab);

        dst[i++] = cur >> 16;
        dst[i++] = (cur >> 8) & 0xff;
        dst[i] = cur & 0xff;
    }
out:
    return rc;
}

bool
unbase64_validate(const std::string &str)
{
    size_t count = 0;

    for (char c: str) {
        if (strchr(tab, c)) {
            ++count;
            continue;
        }
        if (strchr(whitespace, c)) {
            continue;
        }
        if (c == '=') {
            break;
        }
        return false;
    }

    return (count % 4) != 1;
}

bool
unbase64_inplace(std::string &str)
{
    size_t size = str.size();
    size_t inpos = 0, total = 0;
    char inbuf[4096], outbuf[3072];

    for (size_t i = 0; i < size; ++i) {
        if (strchr(tab, str[i])) {
            inbuf[inpos++] = str[i];

            if (inpos == sizeof(inbuf)) {
                unbase64(inbuf, sizeof(inbuf), outbuf);
                str.replace(total, sizeof(outbuf), outbuf, sizeof(outbuf));
                total += sizeof(outbuf);
                inpos = 0;
            }
        }
        else if (strchr(whitespace, str[i]))
            continue;
        else if (str[i] == '=')
            break;
        else
            return false;
    }
    if (inpos) {
        size_t rc = unbase64(inbuf, inpos, outbuf);
        str.replace(total, rc, outbuf, rc);
        total += rc;
    }

    str.resize(total);
    return true;
}

bool
unbase64_inplace_utf8(std::string &str)
{
    return unbase64_inplace(str) &&
        utf8::is_valid(str.cbegin(), str.cend()) &&
        str.find('\0') == std::string::npos;
}

bool
unbase64_inplace_hash(std::string &str, size_t offset, uint64_t &hashret)
{
    size_t size = str.size();
    size_t inpos = 0, total = 0;
    char inbuf[4096], outbuf[3072];

    // Hash function from http://stackoverflow.com/questions/13325125
    hashret = 104395301;

    for (size_t i = offset; i < size; ++i) {
        if (strchr(tab, str[i])) {
            inbuf[inpos++] = str[i];

            if (inpos == sizeof(inbuf)) {
                unbase64(inbuf, sizeof(inbuf), outbuf);
                str.replace(total, sizeof(outbuf), outbuf, sizeof(outbuf));
                total += sizeof(outbuf);
                inpos = 0;

                for (char c: outbuf)
                    hashret += (c * 2654435789) ^ (hashret >> 23);
            }
        }
        else if (strchr(whitespace, str[i]))
            continue;
        else if (str[i] == '=')
            break;
        else
            return false;
    }
    if (inpos) {
        size_t rc = unbase64(inbuf, inpos, outbuf);
        str.replace(total, rc, outbuf, rc);
        total += rc;

        for (size_t i = 0; i < rc; ++i)
            hashret += (outbuf[i] * 2654435789) ^ (hashret >> 23);
    }

    str.resize(total);
    hashret ^= hashret << 37;
    return true;
}
