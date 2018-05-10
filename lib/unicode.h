// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "types.h"
#include "flags.h"

#include <vector>

// Unicode variants
#define TSQ_UNICODE_VARIANT_100             "Unicode 10.0"
#define TSQ_UNICODE_REVISION_100            1

// Encoding boolean parameters (prefixed with + or -)
#define TSQ_UNICODE_PARAM_EMOJI             "emoji"
#define TSQ_UNICODE_PARAM_WIDEAMBIG         "wideambig"

// Recommended default encoding string
// Encoding string consists of a variant name followed by optional version and
// parameter specifications, all separated by \x1f characters
#define TSQ_UNICODE_DEFAULT TSQ_UNICODE_VARIANT_100 "\x1f+" TSQ_UNICODE_PARAM_EMOJI

namespace Tsq
{
    struct UnicodingSpec
    {
        std::string variant;
        int revision;
        bool emoji;
        bool wideambig;

        UnicodingSpec() = default;
        UnicodingSpec(const std::string &name);
        std::string name() const;

        bool operator==(const UnicodingSpec &o) const;
        bool operator!=(const UnicodingSpec &o) const;
    };

    class Unicoding
    {
    protected:
        std::vector<codepoint_t> m_seq;

        CellFlags m_flags = 0;
        CellFlags m_nextFlags;
        std::vector<codepoint_t> m_nextSeq;

        Unicoding() = default;

    public:
        // Factory method
        static Unicoding* create();
        static Unicoding* create(const UnicodingSpec &spec);

        virtual UnicodingSpec spec() const = 0;

    public:
        virtual ~Unicoding() = default;

        inline const auto& seq() const { return m_seq; }

        virtual int widthAt(std::string::const_iterator pos,
                            std::string::const_iterator end) const = 0;
        virtual int widthNext(std::string::const_iterator &posret,
                              std::string::const_iterator end) = 0;
        virtual int widthCategoryOf(codepoint_t c, CellFlags &flagsor) = 0;

        inline void restart(codepoint_t c) {
            m_flags = 0;
            m_seq.assign(1, c);
        }

        virtual void next(std::string::const_iterator &posret,
                          std::string::const_iterator end) = 0;

        inline CellFlags nextFlags() const { return m_nextFlags; }
        virtual std::string nextEmojiName() const;
    };

    inline bool
    UnicodingSpec::operator==(const UnicodingSpec &o) const
    {
        return variant == o.variant && revision == o.revision &&
            emoji == o.emoji && wideambig == o.wideambig;
    }

    inline bool
    UnicodingSpec::operator!=(const UnicodingSpec &o) const
    {
        return variant != o.variant || revision != o.revision ||
            emoji != o.emoji || wideambig != o.wideambig;
    }
}
