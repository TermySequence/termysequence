// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "wait.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <cerrno>

int
osWaitForAny(int *status)
{
    return wait(status);
}

bool
osWaitForChild(int pid, int *status)
{
    return waitpid(pid, status, 0) == 0 || errno != EINTR;
}

bool
osExitedOnSignal(int status)
{
    return WIFSIGNALED(status);
}

int
osExitStatus(int status)
{
    return WEXITSTATUS(status);
}

int
osExitSignal(int status)
{
    return WTERMSIG(status);
}

bool
osDumpedCore(int status)
{
    return WCOREDUMP(status);
}
