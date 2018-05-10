// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "forkpty.h"
#include "conn.h"
#include "dir.h"
#include "lib/argv.h"
#include "lib/exception.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <cstdarg>
#include <unistd.h>
#include <fcntl.h>

int
osFork()
{
    pid_t pid = fork();
    if (pid < 0)
        throw Tsq::ErrnoException("fork", errno);

    return pid;
}

static inline void
osDup2(int oldfd, int newfd)
{
    while (dup2(oldfd, newfd) == -1 && errno == EBUSY);
}

void
osDaemonize()
{
    setsid();
    int fd = open("/dev/null", O_RDWR);
    osDup2(fd, STDIN_FILENO);
    osDup2(fd, STDOUT_FILENO);
    osDup2(fd, STDERR_FILENO);
    close(fd);
}

static void
printStr(const char *buf, size_t len)
{
    while (len) {
        ssize_t rc = write(STDOUT_FILENO, buf, len);
        if (rc <= 0)
            break;

        buf += rc;
        len -= rc;
    }
}

#define printLiteral(x) printStr(x, sizeof(x) - 1)
#define printChar(c) write(STDOUT_FILENO, &c, 1)

static void
printError(const char *str1, const char *str2)
{
    const char *err = strerror(errno);

    printStr(str1, strlen(str1));
    printLiteral(" \"");
    printStr(str2, strlen(str2));
    printLiteral("\": ");
    printStr(err, strlen(err));
    printLiteral("\r\n");
}

int
osForkServer(const char *pathspec, bool standalone, int *pidret)
{
    *pidret = 0;

    // See if we can connect to an existing server
    if (!standalone) {
        int fd = osServerConnect(pathspec);
        if (fd >= 0)
            return fd;
    }

    // Check PATH for server executable
    if (!osFindBin(SERVER_NAME))
        // Let the caller handle this error
        return -1;

    // Create socket pair
    int sd[2];
    osSocketPair(sd);

    // Fork and exec mux program
    switch (*pidret = fork()) {
    case -1:
        close(sd[1]);
        close(sd[0]);
        throw Tsq::ErrnoException("fork", errno);
    case 0:
        close(sd[0]);
        osDup2(sd[1], STDIN_FILENO);
        osDup2(sd[1], STDOUT_FILENO);
        close(sd[1]);

        if (standalone)
            execlp(SERVER_NAME, SERVER_NAME, "--standalone", NULL);
        else
            execlp(SERVER_NAME, SERVER_NAME, NULL);

        _exit(127);
        // noreturn
    }

    close(sd[1]);
    return sd[0];
}

static bool
locateMonitor(std::string &path)
{
    const char *pathc;
    struct stat info;

    if ((osConfigPath(SERVER_NAME, "monitor-script", path) &&
         stat(path.c_str(), &info) == 0 &&
         S_ISREG(info.st_mode) && info.st_mode & S_IXUSR))
    {
        return true;
    }

    if ((stat(pathc = "/etc/" SERVER_NAME "/monitor-script", &info) == 0 &&
         S_ISREG(info.st_mode) && info.st_mode & S_IXUSR))
    {
        path = pathc;
        return true;
    }

    if ((stat(pathc = PREFIX "/lib/" SERVER_NAME "/monitor-script", &info) == 0 &&
         S_ISREG(info.st_mode) && info.st_mode & S_IXUSR))
    {
        path = pathc;
        return true;
    }

    return false;
}

int
osForkMonitor(int *pidret)
{
    // Figure out program to run
    std::string path;
    const char *prog = locateMonitor(path) ? path.c_str() : MONITOR_NAME;

    // Create socket pair
    int sd[2];
    osSocketPair(sd);

    // Fork and exec monitor program
    switch (*pidret = fork()) {
    case -1:
        close(sd[1]);
        close(sd[0]);
        throw Tsq::ErrnoException("fork", errno);
    case 0:
        setpgid(0, 0);
        close(sd[0]);
        osDup2(sd[1], STDIN_FILENO);
        osDup2(sd[1], STDOUT_FILENO);
        close(sd[1]);

        execlp(prog, prog, NULL);

        _exit(127);
        // noreturn
    }

    close(sd[1]);
    return sd[0];
}

int
osForkProcess(const ForkParams &p, int *pidret)
{
    // Build process arguments
    Tsq::ExecArgs args(p.command, p.env);
    const char *dirc = p.dir.c_str();

    // Create socket pair or devnull redirect
    int sd[2];
    if (p.devnull) {
        sd[0] = -1;
        sd[1] = open("/dev/null", O_RDWR);
    } else {
        osSocketPair(sd);
    }

    // Fork and exec
    switch (*pidret = fork()) {
    case -1:
        close(sd[1]);
        close(sd[0]);
        throw Tsq::ErrnoException("fork", errno);
    case 0:
        close(sd[0]);

        // Set blocking
        osMakeBlocking(sd[1]);

        if (p.daemon)
            setsid();
        else
            setpgid(0, 0);

        osDup2(sd[1], STDIN_FILENO);
        osDup2(sd[1], STDOUT_FILENO);
        osDup2(sd[1], STDERR_FILENO);
        close(sd[1]);

        if (chdir(dirc) != 0) {
            printError("chdir", dirc);
            chdir("/");
        }

        environ = (char **)args.env;
        execvp(args.prog, args.vec);

        printError("exec", args.prog);
        _exit(127);
        // noreturn
    }

    close(sd[1]);
    return sd[0];
}

int
osForkTerminal(const PtyParams &p, int *pidret, char *pathret)
{
    int fd;

    // Build process arguments
    Tsq::ExecArgs args(p.command, p.env);
    const char *dirc = p.dir.c_str();

    // Launch process
    if ((*pidret = osForkPty(p, &fd, pathret)) == 0)
    {
        setsid();
        osSetTerminal(fd);
        osDup2(fd, STDIN_FILENO);
        osDup2(fd, STDOUT_FILENO);
        osDup2(fd, STDERR_FILENO);
        close(fd);

        if (p.sleepTime)
            sleep(p.sleepTime);

        if (chdir(dirc) != 0) {
            printError("chdir", dirc);
            chdir("/");
        }

        environ = (char **)args.env;
        execvp(args.prog, args.vec);

        printError("exec", args.vec[0]);
        if (p.exitDelay)
            for (fd = 60; fd > 0; --fd) {
                char c = '\r';
                printChar(c);
                c = '0' + fd / 10;
                printChar(c);
                c = '0' + fd % 10;
                printChar(c);
                sleep(1);
            }
        _exit(127);
        // noreturn
    }

    return fd;
}

int
osForkAttributes(const char *cmdline, int *pidret)
{
    // Build process arguments
    const char *dirc = getenv("HOME");

    // Create socket pair
    int sd[2];
    osSocketPair(sd);

    // Fork and exec
    switch (*pidret = fork()) {
    case -1:
        close(sd[1]);
        close(sd[0]);
        throw Tsq::ErrnoException("fork", errno);
    case 0:
        close(sd[0]);

        // Set blocking
        osMakeBlocking(sd[1]);

        setsid();
        osDup2(sd[1], STDIN_FILENO);
        osDup2(sd[1], STDOUT_FILENO);
        osDup2(sd[1], STDERR_FILENO);
        close(sd[1]);

        if (!dirc || chdir(dirc) != 0)
            chdir("/");

        execl("/bin/sh", "sh", "-c", cmdline, NULL);

        _exit(127);
        // noreturn
    }

    close(sd[1]);
    return sd[0];
}
