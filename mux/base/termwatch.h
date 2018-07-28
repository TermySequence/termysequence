// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "connwatch.h"
#include "eventstate.h"
#include "term.h"

class TermWatch final: public ConnWatch
{
private:
    TermEmulator *m_emulator;

protected:
    void pushAnnounce();
    void pullFiles();

public:
    // locked
    TermEventState state;
    StringMap files;

public:
    TermWatch(TermInstance *parent, TermReader *reader);

    // unlocked
    inline TermInstance* term() { return static_cast<TermInstance*>(m_parent); }
    inline TermEmulator* emulator() { return m_emulator; }

    void pushUserRegion(bufreg_t region);
    void pushDirectoryUpdate(const std::string &msg);
    void pushFileUpdate(const std::string &name, const std::string &msg);
    void pushFileUpdates(const StringMap &map);

    void pullEverything();
};
