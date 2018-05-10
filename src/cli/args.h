// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

class ArgParser
{
private:
    unsigned m_cmd;
    char *const *m_args;
    int m_argc;

    void handleHelp() __attribute__((noreturn));

public:
    void parse(int argc, char **argv);

    inline unsigned cmd() const { return m_cmd; }
    inline auto args() const { return m_args; }
    inline int argc() const { return m_argc; }
};
