// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "uniplugin.h"

// Compiled-in variants
#define TSQ_UNICODE_VARIANT_TERMY           "TermyUnicode"
#define TSQ_UNICODE_REVISION_TERMY          "120"

#define TSQ_UNICODE_VARIANT_SYSWC           "SystemLocale"
#define TSQ_UNICODE_REVISION_SYSWC          "1"

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
    class Unicoding: protected UnicodingImpl
    {
    public:
        // Note: This must be populated in a subclass
        virtual ~Unicoding();

        inline std::string name() const { return UnicodingSpec(params).m_name; }

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

        inline auto nextFlags() const { return UnicodingImpl::nextFlags; }
        inline auto nextLen() const { return UnicodingImpl::nextLen; }
        std::string nextEmojiName() const;
    };
}

// Init functions for compiled-in variants
extern "C" int32_t
uniplugin_termy_init(int32_t, UnicodingInfo*);

extern "C" int32_t
uniplugin_syswc_init(int32_t, UnicodingInfo*);
