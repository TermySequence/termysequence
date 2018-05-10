// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "dir.h"
#include "conn.h"
#include "lib/exception.h"
#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <climits>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>

#ifndef __linux__
#define O_PATH O_RDONLY
#endif

#define ERR1 "XDG_RUNTIME_DIR not set"
#define ERR2 "runtime path exceeds the maximum socket path length of %d"
#define ERR3 "%s not a directory, not owned by UID %d, or not mode 0700"
#define ERR4 "no runtime directory has been created"

static char s_dirpath[SOCKET_PATHLEN];

void
osCreateRuntimeDir(const char *pathspec, char *pathret)
{
    std::string path(pathspec);
    struct stat info;
    size_t idx;
    const char *var;
    uid_t uid = getuid();

    // Perform specifier substitutions
    if ((idx = path.find("%t")) != std::string::npos) {
        var = getenv("XDG_RUNTIME_DIR");
        if (!var)
            throw Tsq::TsqException(ERR1);
        path.replace(idx, 2, var);
    }
    while ((idx = path.find("%U")) != std::string::npos) {
        path.replace(idx, 2, std::to_string(uid));
    }

    if (path.size() + sizeof(SOCKET_FILE) > SOCKET_PATHLEN)
        throw Tsq::TsqException(ERR2, SOCKET_PATHLEN);

    // Create or validate the runtime directory
    var = path.c_str();
    if (lstat(var, &info) == 0) {
        // Make sure it is a directory owned by us
        if (!S_ISDIR(info.st_mode) ||
            info.st_uid != uid ||
            (info.st_mode & 0777) != 0700)
            throw Tsq::TsqException(ERR3, var, uid);
    }
    else {
        if (errno != ENOENT)
            throw Tsq::ErrnoException("stat", var, errno);
        if (mkdir(var, 0700) == -1)
            throw Tsq::ErrnoException("mkdir", var, errno);
    }

    // Save a copy for internal use
    strcpy(s_dirpath, var);
    strcpy(pathret, var);
}

static bool
checkDir(const char *pathspec, char *pathret)
{
    std::string path(pathspec);
    struct stat info;
    size_t idx;
    const char *var;
    uid_t uid = getuid();

    // Perform specifier substitutions
    if ((idx = path.find("%t")) != std::string::npos) {
        var = getenv("XDG_RUNTIME_DIR");
        if (!var) {
            errno = ENOENT;
            return false;
        }
        path.replace(idx, 2, var);
    }
    while ((idx = path.find("%U")) != std::string::npos) {
        path.replace(idx, 2, std::to_string(uid));
    }

    if (path.size() + sizeof(SOCKET_FILE) > SOCKET_PATHLEN)
        throw Tsq::TsqException(ERR2, SOCKET_PATHLEN);

    // Make sure it is a directory owned by us
    var = path.c_str();
    if (lstat(var, &info) != 0) {
        return false;
    }
    if (!S_ISDIR(info.st_mode)) {
        errno = ENOTDIR;
        return false;
    }
    if (info.st_uid != uid || (info.st_mode & 0777) != 0700) {
        errno = EACCES;
        return false;
    }

    strcpy(pathret, var);
    strcat(pathret, SOCKET_FILE);
    return true;
}

static int
doConnect(const char *xdg, const char *run, const char *fallback)
{
    char udspath[SOCKET_PATHLEN];
    int fd;

#if USE_SYSTEMD
    if (checkDir(xdg, udspath) && (fd = osLocalConnect(udspath)) != -1)
        goto out;
    if (checkDir(run, udspath) && (fd = osLocalConnect(udspath)) != -1)
        goto out;
#endif
    if (!checkDir(fallback, udspath))
        return -1;

    fd = osLocalConnect(udspath);
    if (fd != -1) {
    out:
        osLocalCredsCheck(fd);
    }
    return fd;
}

int
osServerConnect(const char *pathspec)
{
    return doConnect(SERVER_XDG_DIR, SERVER_RUN_DIR, pathspec);
}

int
osAppConnect()
{
    return doConnect(APP_XDG_DIR, APP_RUN_DIR, APP_TMP_DIR);
}

void
osCreateSocketPath(char *pathinout)
{
    /*
     * Important: we assume that createRuntimeDir has already been called
     * and the runtime directory is validated and created at this point
     */

    strcat(pathinout, SOCKET_FILE);
    unlink(pathinout);
}

void
osCreatePidFile(char *pathinout)
{
    FILE *file;

    /*
     * Important: we assume that createRuntimeDir has already been called
     * and the runtime directory is validated and created at this point
     */

    strcat(pathinout, PID_FILE);

    file = fopen(pathinout, "w");
    if (file) {
        fprintf(file, "%d\n", getpid());
        fclose(file);
    }
}

void
osRelativeToHome(std::string &path)
{
    if (path.empty() || path.front() != '/') {
        const char *home = getenv("HOME");
        std::string tmp = home ? home : "";
        tmp.push_back('/');
        tmp.append(path);

        while (tmp.size() > 1 && tmp.back() == '/')
            tmp.pop_back();

        path = std::move(tmp);
    }
}

bool
osConfigPath(const char *appname, const char *filename, std::string &result)
{
    const char *xdghome = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");

    if (xdghome)
        result = xdghome;
    else if (home) {
        result = home;
        result.append("/.config");
    }
    else
        return false;

    result.push_back('/');
    result.append(appname);
    result.push_back('/');
    result.append(filename);
    return true;
}

int
osCreateMountPath(const std::string &id, std::string &pathret)
{
    if (!*s_dirpath)
        throw Tsq::TsqException(ERR4);

    pathret.assign(s_dirpath);
    pathret.push_back('/');
    pathret.push_back('m');
    pathret.append(id);

    // Create and open the subdirectory
    const char *pathc = pathret.c_str();
    int fd;

    if (mkdir(pathc, 0700) != 0)
        throw Tsq::ErrnoException("mkdir", pathc, errno);
    if ((fd = open(pathc, O_PATH|O_DIRECTORY|O_CLOEXEC)) < 0)
        throw Tsq::ErrnoException("open", pathc, errno);

    pathret.push_back('/');
    return fd;
}

int
osCreateNamedPipe(bool ro, unsigned mode, std::string &pathret)
{
    assert(*s_dirpath);
    pathret.assign(s_dirpath);
    pathret.push_back('/');
    pathret.push_back(ro ? 'i' : 'o');
    int flag = ro ? O_RDONLY : O_RDWR;

    for (int idx = 1;; ++idx) {
        std::string tmp = pathret + std::to_string(idx);
        const char *tmpc = tmp.c_str();
        int fd;

        if (mkfifo(tmpc, mode) != 0)
            continue;
        if ((fd = open(tmpc, flag|O_NONBLOCK|O_CLOEXEC)) < 0) {
            unlink(tmpc);
            continue;
        }

        pathret = std::move(tmp);
        return fd;
    }
}

static bool
findBinHelper(const char *name, char *pathbuf, size_t pathlen)
{
    strcat(pathbuf, "/");
    strncat(pathbuf, name, pathlen - strlen(pathbuf) - 1);

    return access(pathbuf, X_OK) == 0;
}

bool
osFindBin(const char *name)
{
    char pathbuf[PATH_MAX];
    const size_t pathlen = sizeof(pathbuf);

    strcpy(pathbuf, "/usr/bin");
    if (findBinHelper(name, pathbuf, pathlen))
        return true;
    strcpy(pathbuf, "/bin");
    if (findBinHelper(name, pathbuf, pathlen))
        return true;

    const char *path = getenv("PATH");
    if (path) {
        const char *ptr1 = path, *ptr2;
        size_t len;

        while (true) {
            ptr2 = strchr(ptr1, ':');
            len = pathlen - 2;
            if (ptr2 && ptr2 - ptr1 < len) {
                len = ptr2 - ptr1;
            }
            strncpy(pathbuf, ptr1, len);
            pathbuf[len] = '\0';

            if (findBinHelper(name, pathbuf, pathlen))
                return true;

            if (ptr2)
                ptr1 = ptr2 + 1;
            else
                break;
        }
    }

    return false;
}

int
osMkpath(const std::string &path, unsigned mode)
{
    // Add executable bits to mode
    if (mode & (S_IRUSR|S_IWUSR))
        mode |= S_IXUSR;
    if (mode & (S_IRGRP|S_IWGRP))
        mode |= S_IXGRP;
    if (mode & (S_IROTH|S_IWOTH))
        mode |= S_IXOTH;

    std::string tmp = path;
    size_t idx = 0;
    struct stat info;

    while ((idx = tmp.find('/', idx + 1)) != std::string::npos)
    {
        tmp[idx] = '\0';
        if (stat(tmp.c_str(), &info) == -1)
            if (mkdir(tmp.c_str(), mode) == -1)
                return -1;
        tmp[idx] = '/';
    }
    return 0;
}
