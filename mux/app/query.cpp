// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "args.h"
#include "os/pty.h"
#include "os/conn.h"
#include "lib/exitcode.h"

#include <unistd.h>

static int
doGet(const char *name)
{
    std::string result;
    bool found;

    if (!osGetEmulatorAttribute(name, result, &found))
        return EXITCODE_LISTENERR;
    if (!found)
        return EXITCODE_CONNECTERR;

    osMakeBlocking(STDOUT_FILENO);

    size_t left = result.size();
    const char *ptr = result.data();

    while (left) {
        ssize_t rc = write(STDOUT_FILENO, ptr, left);
        if (rc < 0)
            return EXITCODE_SERVERERR;
        ptr += rc;
        left -= rc;
    }
    return EXITCODE_SUCCESS;
}

int
runQuery()
{
    if (!getenv(ENV_NAME) || !isatty(STDIN_FILENO))
        return EXITCODE_LISTENERR;

    char *termios = nullptr;
    osMakeRawTerminal(STDIN_FILENO, &termios);
    osMakeNonblocking(STDIN_FILENO);

    const auto &cmd = g_args->cmd();
    const char *name = g_args->env().data();
    int rc;

    if (cmd == "set") {
        rc = osSetEmulatorAttribute(name, g_args->arg0()) ?
            EXITCODE_SUCCESS : EXITCODE_LISTENERR;
    } else if (cmd == "clear") {
        rc = osClearEmulatorAttribute(name) ?
            EXITCODE_SUCCESS : EXITCODE_LISTENERR;
    } else {
        rc = doGet(name);
    }

    osRestoreTerminal(STDIN_FILENO, termios);
    delete [] termios;
    return rc;
}
