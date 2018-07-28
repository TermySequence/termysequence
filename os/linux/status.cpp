// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/status.h"
#include "lib/attrstr.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define FD_CWD      0
#define FD_COMM     1
#define FD_EXE      2
#define FD_CMDLINE  3
#define N_FD        4
#define PID_CWD     4
#define PID_COMM    5
#define PID_EXE     6
#define PID_CMDLINE 7
#define N_TOTAL     8

#define INVALID_FD -1
#define INVALID_PID 0

static inline void
checkClose(int *fd)
{
    if (*fd != INVALID_FD) {
        close(*fd);
        *fd = INVALID_FD;
    }
}

void
osStatusInit(int **data)
{
    *data = new int[N_TOTAL];
    int i = 0;

    for (; i < N_FD; ++i)
        (*data)[i] = INVALID_FD;
    for (; i < N_TOTAL; ++i)
        (*data)[i] = INVALID_PID;
}

void
osStatusTeardown(int *data)
{
    for (int i = 0; i < N_FD; ++i)
        checkClose(data + i);

    delete [] data;
}

void
osGetProcessAttributes(int *data, int pid, StringMap &current, StringMap &next)
{
    char path[64], buf[256];
    int pathoff;
    ssize_t rc;
    struct stat info;

    // This string is reused later, keep this line first
    pathoff = sprintf(path, "/proc/%d", pid);
    if (stat(path, &info) == 0) {
        std::string str = std::to_string(info.st_uid);
        if (current[Tsq::attr_PROC_UID] != str)
            next[Tsq::attr_PROC_UID] = str;

        str = std::to_string(info.st_gid);
        if (current[Tsq::attr_PROC_GID] != str)
            next[Tsq::attr_PROC_GID] = str;
    }

// skip0:
    if (data[PID_CWD] != pid) {
        data[PID_CWD] = pid;
        checkClose(data + FD_CWD);
    }
    if (data[FD_CWD] == INVALID_FD) {
        strcpy(path + pathoff, "/cwd");
        data[FD_CWD] = open(path, O_PATH|O_NOFOLLOW|O_CLOEXEC);

        if (data[FD_CWD] == INVALID_FD) {
            data[PID_CWD] = INVALID_PID;
            goto skip1;
        }
    }

    rc = readlinkat(data[FD_CWD], "", buf, sizeof(buf));
    if (rc >= 0) {
        std::string str(buf, rc);
        if (current[Tsq::attr_PROC_CWD] != str)
            next[Tsq::attr_PROC_CWD] = str;
    }

skip1:
    if (data[PID_COMM] != pid) {
        data[PID_COMM] = pid;
        checkClose(data + FD_COMM);
    }
    if (data[FD_COMM] == INVALID_FD) {
        strcpy(path + pathoff, "/comm");
        data[FD_COMM] = open(path, O_RDONLY|O_CLOEXEC);

        if (data[FD_COMM] == INVALID_FD) {
            data[PID_COMM] = INVALID_PID;
            goto skip2;
        }
    }

    lseek(data[FD_COMM], 0, SEEK_SET);
    rc = read(data[FD_COMM], buf, sizeof(buf));
    if (rc >= 0) {
        std::string str(buf, rc);
        if (!str.empty() && str.back() == '\n')
            str.pop_back();
        if (current[Tsq::attr_PROC_COMM] != str)
            next[Tsq::attr_PROC_COMM] = str;
    }

skip2:
    if (data[PID_EXE] != pid) {
        data[PID_EXE] = pid;
        checkClose(data + FD_EXE);
    }
    if (data[FD_EXE] == INVALID_FD) {
        strcpy(path + pathoff, "/exe");
        data[FD_EXE] = open(path, O_PATH|O_NOFOLLOW|O_CLOEXEC);

        if (data[FD_EXE] == INVALID_FD) {
            data[PID_EXE] = INVALID_PID;
            goto skip3;
        }
    }

    rc = readlinkat(data[FD_EXE], "", buf, sizeof(buf));
    if (rc >= 0) {
        std::string str(buf, rc);
        if (current[Tsq::attr_PROC_EXE] != str)
            next[Tsq::attr_PROC_EXE] = str;
    }

skip3:
    if (data[PID_CMDLINE] != pid) {
        data[PID_CMDLINE] = pid;
        checkClose(data + FD_CMDLINE);
    }
    if (data[FD_CMDLINE] == INVALID_FD) {
        strcpy(path + pathoff, "/cmdline");
        data[FD_CMDLINE] = open(path, O_RDONLY|O_CLOEXEC);

        if (data[FD_CMDLINE] == INVALID_FD) {
            data[PID_CMDLINE] = INVALID_PID;
            return;
        }
    }

    lseek(data[FD_CMDLINE], 0, SEEK_SET);
    rc = read(data[FD_CMDLINE], buf, sizeof(buf));
    if (rc > 0) {
        for (int i = 0; i < rc - 1; ++i)
            if (buf[i] == '\0')
                buf[i] = '\x1f';

        std::string str(buf, rc - 1);
        if (current[Tsq::attr_PROC_ARGV] != str)
            next[Tsq::attr_PROC_ARGV] = str;
    }
}
