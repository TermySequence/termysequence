// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

namespace Tsq { class Unicoding; }

class TermLocale
{
private:
    Tsq::Unicoding *m_unicoding;
    uint64_t m_locale = 0;
    std::string m_spec, m_lang;

    Tsq::Unicoding* createEncoding();

public:
    TermLocale(std::string &&spec, std::string &&lang);
    ~TermLocale();

    void setLocale();

    inline const auto& spec() const { return m_spec; }
    inline const auto& lang() const { return m_lang; }

    inline auto unicoding() const { return m_unicoding; }
};
