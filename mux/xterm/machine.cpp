// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "machine.h"
#include "xterm.h"
#include "app/args.h"
#include "os/logging.h"

#include <sstream>

static const Codepoint s_esc = 0x1b;
static const char *const s_escapes = "DEHMNOPVWXZ[\\]^_";
static const char *const s_restarts = "\x1b\x90\x9b\x9d\x9e\x9f";
static const Codestring s_mtcs;

#define TR_ERROR1 TL("server", "Unrecognized control sequence", "error1")

XTermStateMachine::XTermStateMachine(XTermEmulator *emulator,
                                     const Translator *translator) :
    m_node(&m_root),
    m_emulator(emulator),
    m_translator(translator)
{
}

Codestring &
XTermStateMachine::varref(int varnum)
{
    // Unchecked
    return m_vars.find(varnum)->second;
}

const Codestring &
XTermStateMachine::var(int varnum) const
{
    auto i = m_vars.find(varnum);
    auto j = m_vars.end();

    return (i != j) ? i->second : s_mtcs;
}

std::vector<const Codestring*>
XTermStateMachine::varList(int varnum) const
{
    std::vector<const Codestring*> list;
    auto range = m_vars.equal_range(varnum);
    auto i = range.first;
    auto j = range.second;

    while (i != j) {
        list.push_back(&i->second);
        ++i;
    }

    return list;
}

unsigned
XTermStateMachine::varCount(int varnum) const
{
    auto range = m_vars.equal_range(varnum);
    return std::distance(range.first, range.second);
}

void
XTermStateMachine::reset()
{
    m_node = &m_root;

    m_curSequence.clear();
    m_allSequence.clear();
    m_vars.clear();

    m_haveEsc = false;
}

void
XTermStateMachine::next()
{
    m_curSequence.clear();
}

void
XTermStateMachine::push(Codepoint c)
{
    m_curSequence.push_back(c);
    m_allSequence.push_back(c);
}

void
XTermStateMachine::pushVar(int varnum, const Codestring &str)
{
    m_vars.emplace(varnum, str);
}

std::string
XTermStateMachine::dumpState(const char *msg)
{
    std::ostringstream message;
    unsigned limit = 100;

    if (msg && *msg) {
        message << msg << ": ";
    }
    else if (m_allSequence.size() == 1) {
        Codepoint c = m_allSequence.front();
        if (c < 32) {
            message << '^' << (char)('@' + c);
            goto out;
        }
        if (c == 0x7f) {
            message << '^' << '?';
            goto out;
        }
    }

    for (auto i = m_allSequence.begin(), j = m_allSequence.end(); i != j; )
    {
        if (limit-- == 0) {
            message << " ...";
            break;
        }

        if (*i >= 0x20 && *i <= 0x7e)
            message << (char)*i;
        else
            message << '(' << *i << ')';

        if (++i != j)
            message << ' ';
    }
out:
    return message.str();

    // for (const auto &i: m_vars)
    //     LOGDBG("\t%d \"%s\"\n", i.first, i.second.c_str());
}

void
XTermStateMachine::call(const XTermNode *node)
{
    XTermHandler slot = node->slot();

    // const char *slotName = node->slotName();
    // if (*slotName != 'p') // process
    // LOGDBG("%s\n", dumpState(slotName).c_str());

    (m_emulator->*slot)();
}

void
XTermStateMachine::dumpError(const char *msg, bool restart)
{
    std::string result = dumpState(msg);

    if (restart) {
        reset();
    };

    m_emulator->internalError(result.c_str());
}

void
XTermStateMachine::processMain(Codepoint c)
{
    XTermEdge *e = m_node->process(this, c);

    if (e == nullptr) {
        m_allSequence.push_back(c);
        dumpError(TR_ERROR1, true);
    }
    else {
        switch (e->process(this, c))
        {
        case XTermEdge::Move:
            m_node = e->next;
            if (m_node->isLeaf) {
                call(m_node);
                reset();
            }
            break;
        case XTermEdge::Call:
            call(e->next);
            m_curSequence.pop_back();
            m_allSequence.pop_back();
            break;
        case XTermEdge::Skip:
            m_node = e->next;
            processMain(c);
            break;
        case XTermEdge::Reset:
            // dumpState("Reset");
            reset();
            break;
        case XTermEdge::Restart:
            // dumpState("Restart");
            reset();
            process(c);
            break;
        default:
            break;
        }
    }
}

inline bool
XTermStateMachine::isEscapeCode(Codepoint c)
{
    const unsigned char *ptr = (const unsigned char *)s_escapes;

    while (*ptr)
        if (*ptr++ == c)
            return true;

    return false;
}

void
XTermStateMachine::process(Codepoint c)
{
/*
    if (c >= 0x20 && c <= 0x7e) {
        LOGDBG("got %c\n", c);
    } else if (c != 0) {
        LOGDBG("got (%d)\n", c);
    }
*/
    // Ignore NULs
    if (c == 0)
        return;

    // Escape sequence processing
    if (m_haveEsc) {
        m_haveEsc = false;
        if (isEscapeCode(c)) {
            processMain(c + 0x40);
        } else {
            processMain(s_esc);
            processMain(c);
        }
    }
    else if (c == s_esc) {
        m_haveEsc = true;
    }
    else {
        processMain(c);
    }
}

bool
XTermStateMachine::isRestartCode(Codepoint c)
{
    const unsigned char *ptr = (const unsigned char *)s_restarts;

    while (*ptr)
        if (*ptr++ == c)
            return true;

    return false;
}
