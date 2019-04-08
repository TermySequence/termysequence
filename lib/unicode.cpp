// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "unicode.h"

#include <map>
#include <cstdio>

namespace Tsq
{
    //
    // UnicodingSpec
    //
    void
    UnicodingSpec::parse()
    {
        variant = m_spec.data();

        std::map<std::string_view,const char *> pmap;
        const char *buf = m_spec.data() + strlen(variant) + 1;
        const char *end = m_spec.data() + m_spec.size();

        while (buf < end) {
            const char *ptr = strchr(buf, '=');
            if (ptr) {
                pmap[std::string_view(buf, ptr - buf)] = buf;
            } else {
                pmap[buf] = buf;
            }
            buf += strlen(buf) + 1;
        }

        m_name = variant;

        params = new const char*[pmap.size() + 1]{};
        int i = 0;
        for (const auto &elt: pmap) {
            params[i++] = elt.second;

            m_name.push_back('\x1f');
            m_name.append(elt.second);
        }
    }

    UnicodingSpec::UnicodingSpec(const std::string &spec) :
        m_spec(spec)
    {
        for (char &c: m_spec)
            if (c == '\x1f')
                c = '\0';
        parse();
    }

    UnicodingSpec::UnicodingSpec(const UnicodingParams &params) :
        m_spec(params.variant)
    {
        const char **p = params.params;
        if (p) {
            while (*p) {
                m_spec.push_back('\0');
                m_spec.append(*p++);
            }
        }
        parse();
    }

    UnicodingSpec::~UnicodingSpec()
    {
        delete [] params;
    }

    //
    // Unicoding
    //
    Unicoding::~Unicoding()
    {
        UnicodingImpl::teardown(this);
    }

    std::string
    Unicoding::nextEmojiName() const
    {
        std::string result;
        char buf[16];

        for (unsigned i = 0, n = UnicodingImpl::nextLen; i < n; ++i) {
            if (i == 1 && nextSeq[i] == EMOJI_SELECTOR)
                continue;

            size_t rc = snprintf(buf, sizeof(buf), "%x-", nextSeq[i]);
            result.append(buf, rc);
        }

        if (!result.empty())
            result.pop_back();

        return result;
    }
}
