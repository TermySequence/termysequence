// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "threadbase.h"

#include <unordered_map>

class TermReaper final: public ThreadBase
{
private:
    bool m_timeToDie;

    std::unordered_map<int,ThreadBase*> m_registered;
    std::unordered_map<int,int> m_orphans;

    void lockLoop();
    void threadMain();

public:
    TermReaper(const std::vector<int> &ignorepids);
    void stop(int reason);

    void registerProcess(ThreadBase *owner, int pid);
    void ignoreProcess(int pid);
    void abandonProcess(int pid);
};

enum ReaperWork {
    ReaperProcessExited = 6, // ProcessExited common value
};

extern TermReaper *g_reaper;
