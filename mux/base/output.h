// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "threadbase.h"
#include <queue>

class ConnInstance;

class TermOutput final: public ThreadBase
{
private:
    ConnInstance *m_parent;

    std::queue<std::string> m_data, c_data;
    std::queue<std::string> m_commands, c_commands;
    size_t m_bufferedAmount;

    bool m_throttled;
    bool m_stopping;

    void lockLoop();
    void threadMain();

public:
    TermOutput(ConnInstance *parent);
    void stop(int reason);
    void reset();

    size_t bufferCurrentAmount() const;
    size_t bufferWarnAmount() const;

    bool submitData(std::string &&buf);
    bool submitCommand(std::string &&buf);

    void writeFd(int fd, const char *buf, size_t len);
};
