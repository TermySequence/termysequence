// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "unicode.h"

using namespace std;

#include "unicode10impl.hpp"

namespace Tsq
{
    UnicodingSpec::UnicodingSpec(const std::string &spec)
    {
        revision = 0;
        emoji = false;
        wideambig = false;

        size_t size = spec.size(), end = spec.find('\x1f');
        if (end == string::npos)
            end = size;

        variant = spec.substr(0, end);

        while (end != size) {
            size_t start = end + 1;
            end = spec.find('\x1f', start);
            if (end == string::npos)
                end = size;
            if (end - start < 3)
                continue;

            char c = spec[start++];

            switch (c) {
            case 'v':
                revision = atoi(spec.substr(start, end).c_str());
                break;
            case '+':
            case '-':
                if (!spec.compare(start, end - start, TSQ_UNICODE_PARAM_EMOJI))
                    emoji = (c == '+');
                else if (!spec.compare(start, end - start, TSQ_UNICODE_PARAM_WIDEAMBIG))
                    wideambig = (c == '+');
            }
        }
    }

    std::string
    UnicodingSpec::name() const
    {
        std::string result = variant;
        result.push_back('\x1f');
        result.push_back('v');
        result.append(std::to_string(revision));

        if (emoji) {
            result.push_back('\x1f');
            result.push_back('+');
            result.append(TSQ_UNICODE_PARAM_EMOJI);
        }
        if (wideambig) {
            result.push_back('\x1f');
            result.push_back('+');
            result.append(TSQ_UNICODE_PARAM_WIDEAMBIG);
        }

        return result;
    }

    Unicoding *
    Unicoding::create(const UnicodingSpec &spec)
    {
        if (spec.emoji)
            return new Tsq_Unicode10::Emoji(spec.wideambig);
        else
            return new Tsq_Unicode10::Text(spec.wideambig);
    }

    Unicoding *
    Unicoding::create()
    {
        return new Tsq_Unicode10::Emoji;
    }

    string
    Unicoding::nextEmojiName() const
    {
        return string();
    }
}
