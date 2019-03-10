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
        revision = 0;
        variant = m_spec.data();

        std::map<std::string_view,const char *> pmap;
        const char *buf = m_spec.data() + strlen(variant) + 1;
        const char *end = m_spec.data() + m_spec.size();

        while (buf < end) {
            switch (*buf) {
            case 'v':
                revision = atoi(buf + 1);
                break;
            case '+':
            case '-':
                pmap[buf + 1] = buf;
                break;
            default:
                pmap[buf] = buf;
                break;
            }
            buf += strlen(buf) + 1;
        }

        m_name = variant;
        m_name.push_back('\x1f');
        m_name.push_back('v');
        m_name.append(std::to_string(revision));

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
        m_spec.push_back('\0');
        m_spec.push_back('v');
        m_spec.append(std::to_string(params.revision));

        const char **p = params.params;
        while (*p) {
            m_spec.push_back('\0');
            m_spec.append(*p++);
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

    std::vector<std::unique_ptr<UnicodingInfo>> Unicoding::m_plugins;
    std::map<std::string_view,UnicodingCreateFunc> Unicoding::m_variants;

    Unicoding *
    Unicoding::create()
    {
        auto *result = new Unicoding();
        UnicodingSpec spec(m_plugins[0]->defaultName);
        m_plugins[0]->create(UNIPLUGIN_VERSION, &spec, result);
        return result;
    }

    Unicoding *
    Unicoding::create(const UnicodingSpec &spec)
    {
        const auto i = m_variants.find(spec.variant);
        if (i != m_variants.cend()) {
            auto *result = new Unicoding();
            if ((*i->second)(UNIPLUGIN_VERSION, &spec, result) == 0)
                return result;
            else
                delete result;
        }
        return create();
    }

    void
    Unicoding::registerPlugin(UnicodingInitFunc func)
    {
        auto info = std::make_unique<UnicodingInfo>();
        if ((*func)(UNIPLUGIN_VERSION, info.get()) == 0) {
            for (const auto *i = info->variants; i->variant; ++i)
                m_variants.emplace(i->variant, info->create);
            m_plugins.emplace_back(std::move(info));
        }
    }

    std::string
    Unicoding::nextEmojiName() const
    {
        std::string result;
        char buf[16];

        for (unsigned i = 0, n = nextLen; i < n; ++i) {
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
