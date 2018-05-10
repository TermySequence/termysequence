// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "baseproxy.h"

#include <atomic>

class ServerProxy final: public BaseProxy
{
private:
    unsigned m_version;
    unsigned m_startingTerms;
    std::atomic_uint m_nTerms;

public:
    ServerProxy(ConnInstance *parent, const char *body, uint32_t length);

    inline unsigned version() const { return m_version; }
    inline unsigned nTerms() const { return m_nTerms; }
    void addTerm();
    inline void removeTerm() { --m_nTerms; }

    void wireCommand(uint32_t command, uint32_t length, const char *body);
};
