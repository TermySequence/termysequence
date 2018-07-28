// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "attributemap.h"

class Translator;

class TermStatusTracker
{
private:
    int *p_os;

    unsigned status;
    int pid;
    unsigned m_outcome;
    int m_exitcode;
    uint32_t termiosFlags[3];
    char termiosChars[20];

    StringMap m_all;
    StringMap m_changed;

    const Translator *m_translator;
    std::string m_outcomeStr;

    void setTermiosInfo();
    inline unsigned wireStatus() const { return status | m_outcome << 8; }

public:
    TermStatusTracker();
    TermStatusTracker(const Translator *translator);
    ~TermStatusTracker();

    inline StringMap& changedMap() { return m_changed; }
    inline unsigned outcome() const { return m_outcome; }
    inline int exitcode() const { return m_exitcode; }
    inline const char* outcomeStr() const { return m_outcomeStr.c_str(); }

    inline bool autoClose(unsigned setting) const { return setting >= m_outcome; }

    void updateOnce(int fd, int primary, const char *title);
    bool update(int fd, int primary);

    void start(const std::string &command, const std::string &dir);
    void setOutcome(int pid, int status);

    static void sendSignal(int fd, int signal);
};
