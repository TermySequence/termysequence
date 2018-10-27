// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/attr.h"
#include "os/status.h"
#include "lib/attrstr.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

struct LinuxStatusState {
    int pid = 0;
    int fd_cwd = -1;
    int fd_comm = -1;
    int fd_cmdline = -1;
};

static inline void
checkClose(int *fd)
{
    if (*fd != -1) {
        close(*fd);
        *fd = -1;
    }
}

void
osStatusInit(void **data)
{
    *data = new LinuxStatusState;
}

void
osStatusTeardown(void *data)
{
    auto *state = static_cast<LinuxStatusState*>(data);

    if (state->fd_cwd != -1)
        close(state->fd_comm);
    if (state->fd_comm != -1)
        close(state->fd_comm);
    if (state->fd_cmdline != -1)
        close(state->fd_cmdline);

    delete state;
}

void
osGetProcessAttributes(void *data, int pid, StringMap &current, StringMap &next)
{
    auto *state = static_cast<LinuxStatusState*>(data);
    char path[64], buf[256];
    int pathoff;
    ssize_t rc;
    struct stat info;

    // This string is reused later, keep this line first
    pathoff = sprintf(path, "/proc/%d", pid);
    if (stat(path, &info) == 0) {
        std::string str = std::to_string(info.st_uid);
        if (current[Tsq::attr_PROC_UID] != str)
            next[Tsq::attr_PROC_UID] = std::move(str);

        str = std::to_string(info.st_gid);
        if (current[Tsq::attr_PROC_GID] != str)
            next[Tsq::attr_PROC_GID] = std::move(str);
    }

    if (state->pid != pid) {
        state->pid = pid;
        checkClose(&state->fd_cwd);
        checkClose(&state->fd_comm);
        checkClose(&state->fd_cmdline);
    }

// proc_cwd:
    if (state->fd_cwd == -1) {
        strcpy(path + pathoff, "/cwd");
        if ((state->fd_cwd = open(path, O_PATH|O_NOFOLLOW|O_CLOEXEC)) == -1)
            goto proc_comm;
    }

    rc = readlinkat(state->fd_cwd, "", buf, sizeof(buf));
    if (rc >= 0) {
        std::string str(buf, rc);
        if (current[Tsq::attr_PROC_CWD] != str)
            next[Tsq::attr_PROC_CWD] = std::move(str);
    }

proc_comm:
    if (state->fd_comm == -1) {
        strcpy(path + pathoff, "/comm");
        if ((state->fd_comm = open(path, O_RDONLY|O_CLOEXEC)) == -1)
            goto proc_cmdline;
    }

    lseek(state->fd_comm, 0, SEEK_SET);
    rc = read(state->fd_comm, buf, sizeof(buf));
    if (rc >= 0) {
        rc -= (rc && buf[rc - 1] == '\n');
        std::string str(buf, rc);
        if (current[Tsq::attr_PROC_COMM] != str)
            next[Tsq::attr_PROC_COMM] = std::move(str);
    }

proc_cmdline:
    if (state->fd_cmdline == -1) {
        strcpy(path + pathoff, "/cmdline");
        if ((state->fd_cmdline = open(path, O_RDONLY|O_CLOEXEC)) == -1)
            return;
    }

    lseek(state->fd_cmdline, 0, SEEK_SET);
    rc = read(state->fd_cmdline, buf, sizeof(buf));
    if (rc > 0) {
        for (int i = 0; i < rc - 1; ++i)
            if (buf[i] == '\0')
                buf[i] = '\x1f';

        std::string str(buf, rc - 1);
        if (current[Tsq::attr_PROC_ARGV] != str)
            next[Tsq::attr_PROC_ARGV] = std::move(str);
    }
}

StringMap
osGetProcessEnvironment(int pid)
{
    char path[64];
    StringMap result;

    sprintf(path, "/proc/%d/environ", pid);
    osLoadFile(path, result, true);
    return result;
}
