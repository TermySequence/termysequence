// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "argv.h"

namespace Tsq
{
    void
    ExecArgs::handleEnvReplace(const char *rule)
    {
        const char *stem = strchr(rule, '=');
        if (stem) {
            size_t n = stem - rule + 1;

            for (auto &&i: m_env) {
                if (!strncmp(stem, i, n)) {
                    i = rule;
                    return;
                }
            }

            m_env.push_back(rule);
        }
    }

    void
    ExecArgs::handleEnvRemove(const char *rule)
    {
        std::string tmp(rule);
        tmp.push_back('=');
        const char *stem = tmp.c_str();
        size_t n = tmp.size();

        for (auto i = m_env.begin(), j = m_env.end(); i != j; ++i)
            if (!strncmp(stem, *i, n)) {
                m_env.erase(i);
                break;
            }
    }

    inline void
    ExecArgs::handleEnvRule(const char *rule)
    {
        if (*rule == '+')
            handleEnvReplace(rule + 1);
        else if (*rule == '-')
            handleEnvRemove(rule + 1);
    }

    ExecArgs::ExecArgs(const std::string &command, const std::string &envrules)
    {
        const char *envc = envrules.c_str();
        size_t i;
        unsigned n;

        // Set up environment block
        if (envrules.empty()) {
            env = environ;
        } else {
            for (char **envptr = environ; *envptr; ++envptr)
                m_env.push_back(*envptr);

            handleEnvRule(envc);

            for (i = 0; i < envrules.size(); ++i)
                if (envc[i] == '\0')
                    handleEnvRule(envc + i + 1);

            m_env.push_back(nullptr);
            env = (char *const *)m_env.data();
        }

        // Split command into arguments
        prog = command.c_str();

        for (i = 0, n = 1; i < command.size(); ++i)
            if (prog[i] == '\0')
                ++n;

        if (n == 1) {
            m_vec.push_back(prog);
        } else {
            for (i = 0, n = 0; i < command.size(); ++i)
                if (prog[i] == '\0')
                    m_vec.push_back(prog + i + 1);
        }

        m_vec.push_back(nullptr);
        vec = (char *const *)m_vec.data();
    }
}
