// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "uniplugin.h"

#include <vector>

// Compiled-in variants
#define TSQ_UNICODE_VARIANT_TERMY           "TermyUnicode"
#define TSQ_UNICODE_REVISION_TERMY          "120"

// Recommended default encoding string
// Encoding string consists of a variant name followed by optional
// parameter strings, all separated by \x1f characters
#define TSQ_UNICODE_DEFAULT TSQ_UNICODE_VARIANT_TERMY

namespace Tsq
{
    //
    // UnicodingSpec
    //
    class UnicodingSpec: public UnicodingParams
    {
        friend class Unicoding;

    private:
        std::string m_spec;
        std::string m_name;

        void parse();

    public:
        UnicodingSpec(const std::string &spec);
        UnicodingSpec(const UnicodingParams &params);
        ~UnicodingSpec();

        inline bool operator==(const UnicodingSpec &o) const
            { return m_name == o.m_name; }
        inline bool operator!=(const UnicodingSpec &o) const
            { return m_name != o.m_name; }
    };

    //
    // Unicoding
    //
    class Unicoding: private UnicodingImpl
    {
    private:
        Unicoding() = default;

    private:
        static std::vector<UnicodingVariant> m_variants;

    public:
        // Factory method
        static Unicoding* create();
        static Unicoding* create(const UnicodingSpec &spec);
        static bool needsLocale(const UnicodingSpec &spec);

        static void registerPlugin(UnicodingInitFunc func);

        inline static const auto& variants() { return m_variants; }

    public:
        ~Unicoding();

        inline UnicodingSpec spec() const { return params; }
        inline std::string name() const { return spec().m_name; }

        // Operations
        inline int widthAt(const char *pos, const char *end) const
            { return UnicodingImpl::widthAt(this, pos, end); }
        inline int widthNext(const char *&posret, const char *end)
            { return UnicodingImpl::widthNext(this, &posret, end); }
        inline int widthCategoryOf(codepoint_t c, CellFlags &flagsor)
            { return UnicodingImpl::widthCategoryOf(this, c, &flagsor); }

        inline void getSeq(const codepoint_t *&i, const codepoint_t *&j) {
            i = seq;
            j = seq + len;
        }
        inline void restart(codepoint_t c) {
            state = 0;
            flags = 0;
            len = 1;
            seq[0] = c;
        }

        inline CellFlags nextFlags() const { return UnicodingImpl::nextFlags; }
        std::string nextEmojiName() const;
    };
}
