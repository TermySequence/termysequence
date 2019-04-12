// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/unicode.h"

#include <vector>

//
// Encoding factory with locale support
//

class TermUnicoding final: public Tsq::Unicoding
{
    private:
        uint64_t m_locale = 0;
        const std::string m_spec, m_lang;

        void populate(const std::string *spec);

    private:
        static std::vector<UnicodingVariant> s_variants;

        static bool needsLocale(const char *prefix);

    public:
        static void registerPlugin(UnicodingInitFunc func);

        inline static const auto& variants() { return s_variants; }

    public:
        TermUnicoding();
        TermUnicoding(const std::string &spec);
        TermUnicoding(std::string &&spec, std::string &&lang);
        ~TermUnicoding();

        inline const auto& spec() const { return m_spec; }
        inline const auto& lang() const { return m_lang; }

        void setLocale();
};
