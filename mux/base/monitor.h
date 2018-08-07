// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "threadbase.h"
#include "attributemap.h"

class TermMonitor final: public ThreadBase
{
private:
    int m_state = 0;
    bool m_restarting = false;

    std::string m_accum;
    StringMap m_map;

private:
    void startMonitor();
    void addSpec(std::string &key, std::string &value);
    void removeSpec(std::string &key);
    void stateChange();

    void threadMain();
    bool handleFd();
    void handleSpec(const char *spec);

    bool handleWork(const WorkItem &item);
    void handleRestart();
    void handleInput(std::string *data);

public:
    TermMonitor();
};

enum MonitorWork {
    MonitorClose,
    MonitorRestart,
    MonitorInput,
};

extern TermMonitor *g_monitor;
