// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "attributemap.h"

class Translator;
struct ForkParams;

class TermStatusTracker
{
private:
    void *p_os;

    unsigned status;
    int pid;
    int m_disposition;
    unsigned m_outcome;
    int m_exitcode;
    uint32_t termiosFlags[3];
    char termiosChars[20];

    StringMap m_all;
    StringMap m_changed;

    SharedStringMap m_environ;
    std::unordered_map<std::string,std::pair<bool,std::string>> m_sessenv;

    const Translator *m_translator;
    std::string m_outcomeStr;

    void setTermiosInfo();
    void handleEnvRule(const char *rule, std::string &envbuf);

    inline unsigned wireStatus() const { return status | m_outcome << 8; }

public:
    TermStatusTracker();
    TermStatusTracker(const Translator *translator, SharedStringMap &&environ);
    ~TermStatusTracker();

    inline StringMap& changedMap() { return m_changed; }
    inline unsigned outcome() const { return m_outcome; }
    inline int exitcode() const { return m_exitcode; }
    inline const char* outcomeStr() const { return m_outcomeStr.c_str(); }

    inline bool autoClose(unsigned setting) const { return setting >= m_outcome; }

    void updateOnce(int fd, int primary, const char *title);
    bool update(int fd, int primary);
    const char* updateOutcome();

    void start(ForkParams *params);
    void setOutcome(int pid, int status);
    bool setEnviron(SharedStringMap &environ);
    void resetEnviron();

    static void sendSignal(int fd, int signal);
};
