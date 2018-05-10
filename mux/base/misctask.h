// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "taskbase.h"

class FileMisc final: public TaskBase
{
private:
    bool m_rename;
    std::string m_destName;

private:
    void reportStatus(Tsq::TaskStatus status);

    bool runRemove();
    bool runRename();

    void threadMain();
    bool handleWork(const WorkItem &item);
    bool handleAnswer(int answer);
    bool handleIdle();

public:
    FileMisc(Tsq::ProtocolUnmarshaler *unm, bool rename);
};
