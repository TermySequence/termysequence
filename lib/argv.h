// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <vector>

extern "C" {
extern char **environ;
}

namespace Tsq
{
    class ExecArgs
    {
    private:
        std::vector<const char*> m_vec, m_env;

        void handleEnvReplace(const char *rule);
        void handleEnvRemove(const char *rule);
        void handleEnvRule(const char *rule);

    public:
        const char *prog;
        char *const *vec;
        char *const *env;

        ExecArgs(const std::string &command, const std::string &envrules);
    };
}
