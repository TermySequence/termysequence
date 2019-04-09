// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

namespace Tsq { class Unicoding; }

class TermLocale
{
private:
    std::string m_spec, m_lang;
    uint64_t m_locale = 0;

public:
    TermLocale(std::string &&spec, std::string &&lang);
    ~TermLocale();

    Tsq::Unicoding* createEncoding();
    void setLocale();
};
