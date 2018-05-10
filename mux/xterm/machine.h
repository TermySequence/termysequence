// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/codestring.h"
#include "graph.h"

#include <map>
#include <vector>

class XTermEmulator;
class XTermNode;
class Translator;

class XTermStateMachine
{
private:
    XTermRootNode m_root;
    XTermNode *m_node;

    XTermEmulator *m_emulator;

    Codestring m_curSequence;
    Codestring m_allSequence;
    std::multimap<int,Codestring> m_vars;
    bool m_haveEsc = false;

    const Translator *m_translator;

    void processMain(Codepoint c);

public:
    XTermStateMachine(XTermEmulator *emulator, const Translator *translator);
    void reset();

    static bool isEscapeCode(Codepoint c);
    static bool isControlCode(Codepoint c);
    static bool isRestartCode(Codepoint c);

    inline const Codestring& allSequence() { return m_allSequence; };
    inline const Codestring& curSequence() { return m_curSequence; };

    const Codestring& var(int varnum) const;
    std::vector<const Codestring*> varList(int varnum) const;
    unsigned varCount(int varnum) const;
    Codestring& varref(int varnum);

    void push(Codepoint c);
    void pushVar(int varnum, const Codestring &str);
    void next();

    std::string dumpState(const char *msg);
    void dumpError(const char *msg, bool restart);

    void process(Codepoint c);
    void call(const XTermNode *node);
};

inline bool
XTermStateMachine::isControlCode(Codepoint c)
{
    return (c <= 0x1f) || (c >= 0x7f && c <= 0x9f);
}
