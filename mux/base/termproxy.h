// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "baseproxy.h"
#include "eventstate.h"

class TermProxy final: public BaseProxy, public ProxyAccumulatedState
{
private:
    bool m_isTerm;

public:
    TermProxy(ConnInstance *parent, const char *body, uint32_t length, bool isTerm);

    inline bool isTerm() const { return m_isTerm; }

    void wireTermFlagsChanged(const char *body, uint32_t length);
    void wireTermBufferCapacity(const char *body, uint32_t length);
    void wireTermBufferLength(const char *body, uint32_t length);
    void wireTermBufferSwitched(const char *body, uint32_t length);
    void wireTermSizeChanged(const char *body, uint32_t length);
    void wireTermCursorMoved(const char *body, uint32_t length);
    void wireTermBellRang(const char *body, uint32_t length);
    void wireTermRowChanged(const char *body, uint32_t length);
    void wireTermRegionChanged(const char *body, uint32_t length);
    void wireTermDirectoryUpdate(const char *body, uint32_t length);
    void wireTermFileUpdate(const char *body, uint32_t length);
    void wireTermFileRemoved(const char *body, uint32_t length);
    void wireTermEndOutput();
    void wireTermMouseMoved(const char *body, uint32_t length);
    void wireCommand(uint32_t command, uint32_t length, const char *body);
};
