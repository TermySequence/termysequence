// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "fd.h"
#include "config.h"
#include "lib/exception.h"
#include "lib/attrstr.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <fts.h>
#include <vector>

int
osOpenFile(const char *name, size_t *sizeret, uint32_t *moderet)
{
    int fd = open(name, O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOCTTY);
    if (fd < 0)
        goto err;

    struct stat info;
    if (fstat(fd, &info) != 0) {
        goto err2;
    }
    if (S_ISDIR(info.st_mode)) {
        errno = EISDIR;
        goto err2;
    }

    if (sizeret)
        *sizeret = info.st_size;
    if (moderet)
        *moderet = info.st_mode & 0777;

    return fd;
err2:
    close(fd);
err:
    throw Tsq::ErrnoException("open", name, errno);
}

int
osOpenDir(const char *path, DIR **dirret, bool blocking)
{
    int flag = blocking ? 0 : O_NONBLOCK;
    int fd = open(path, O_DIRECTORY|O_RDONLY|O_CLOEXEC|flag);
    if (fd < 0)
        return -1;

    *dirret = fdopendir(fd);
    if (!*dirret) {
        close(fd);
        return -1;
    }

    return fd;
}

int
osStatFile(const char *name, FileInfo *info)
{
    struct stat buf;
    if (fstatat(info->dirfd, name, &buf, AT_SYMLINK_NOFOLLOW|AT_NO_AUTOMOUNT) == -1)
        return -1;

    info->mode = buf.st_mode & 0177777;
    info->uid = buf.st_uid;
    info->gid = buf.st_gid;
    info->mtime = buf.m_sec_field * 1000 + buf.m_nsec_field / 10000000;
    info->size = buf.st_size;

    if (S_ISLNK(buf.st_mode)) {
        // Read the link
        char link[1024];
        ssize_t rc = readlinkat(info->dirfd, name, link, sizeof(link));
        if (rc < 0 || rc == sizeof(link))
            rc = 0;

        link[rc] = '\0';
        info->fattr.emplace(Tsq::attr_FILE_LINK, link);

        // Check for broken link
        if (fstatat(info->dirfd, link, &buf, AT_NO_AUTOMOUNT) == 0) {
            char tmp[16];
            snprintf(tmp, sizeof(tmp), "%o", buf.st_mode & 0177777);
            info->fattr.emplace(Tsq::attr_FILE_LINKMODE, tmp);
        } else {
            info->fattr.emplace(Tsq::attr_FILE_ORPHAN, "");
        }
    }

    // 1 if eligible for git status checking, 0 otherwise
    return (S_ISLNK(buf.st_mode) || S_ISREG(buf.st_mode)) ? 1 : 0;
}

int
osReadDir(DIR *dir, FileInfo *info)
{
    struct dirent *ent = readdir(dir);

    if (ent == NULL)
        return -1;

    info->name.assign(ent->d_name);
    return osStatFile(ent->d_name, info) + 1;
}

bool
osFileExists(const char *path, bool *isdirret)
{
    struct stat buf;

    if (stat(path, &buf) != 0)
        return false;

    if (isdirret)
        *isdirret = S_ISDIR(buf.st_mode);

    return true;
}

bool
osFindDirUpwards(std::string &path, const char *dir)
{
    struct stat buf;
    if (stat(path.c_str(), &buf) != 0)
        return false;

    auto fsdev = buf.st_dev;

    while (1) {
        while (!path.empty() && path.back() == '/') {
            path.pop_back();
        }
        if (path.empty()) {
            return false;
        }

        std::string tmp = path + dir;
        if (stat(tmp.c_str(), &buf) == 0) {
            if (buf.st_dev != fsdev)
                return false;
            if (S_ISDIR(buf.st_mode))
                break;
        }

        size_t idx = path.find_last_of('/');
        if (idx == std::string::npos)
            idx = 0;
        else
            ++idx;

        path.erase(idx);
    }

    path.push_back('/');
    return true;
}

int
osRecursiveDelete(const std::string &path)
{
    char *copy = strdup(path.c_str());
    char *ptr[2] = { copy, 0 };
    FTS *fts = fts_open(ptr, FTS_PHYSICAL|FTS_NOSTAT|FTS_NOCHDIR, NULL);
    FTSENT *e;

    if (!fts)
        goto out;

    while ((e = fts_read(fts))) {
        switch (e->fts_info) {
        case FTS_DNR:
            if (e->fts_errno == ENOENT)
                break;
            // fallthru
        case FTS_DP:
        case FTS_F:
        case FTS_NS:
        case FTS_NSOK:
        case FTS_SL:
        case FTS_SLNONE:
            if (remove(e->fts_accpath) != 0 && errno != ENOENT)
                goto out;
            break;
        case FTS_ERR:
            errno = e->fts_errno;
            goto out;
        default:
            break;
        }
    }
out:
    free(copy);
    return errno ? -1 : 0;
}

int
osOpenRenamedFile(std::string &pathret, unsigned mode, bool nofd)
{
    std::string part1, part2;
    int fd, idx = pathret.find_last_of('.');
    if (idx == std::string::npos)
        idx = pathret.size();

    part1 = pathret.substr(0, idx) + '.';
    part2 = pathret.substr(idx);

    for (idx = 1;; ++idx) {
        std::string tmp = part1 + std::to_string(idx) + part2;
        fd = open(tmp.c_str(), O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC, mode);

        if (fd >= 0 || errno != EEXIST) {
            pathret = std::move(tmp);
            break;
        }
    }

    if (nofd)
        close(fd);

    return fd;
}

void
osPurgeFileDescriptors(const char *program)
{
    const struct dirent *file;
    std::vector<int> leaks;

    DIR *dir = opendir("/proc/self/fd");
    if (dir == NULL) {
        for (int i = 3; i < OS_DESCRIPTOR_COUNT; ++i)
            close(i);
        return;
    }
    int dd = dirfd(dir);

    while ((file = readdir(dir)) != NULL)
    {
        const char *name = file->d_name;
        if (file->d_type != DT_LNK || strlen(name) != strspn(name, "0123456789"))
            continue;

        int fd = atoi(name);
        if (fd > 2 && fd != dd)
            leaks.push_back(fd);
    }

#ifdef NDEBUG
    for (int fd: leaks) {
        close(fd);
    }
#else
    if (!leaks.empty()) {
        // Write a report
        char buf[256], num[24];
        sprintf(buf, "/tmp/%s.%d.leakedfds", program, getuid());
        FILE *logfile = fopen(buf, "a");
        fputs("--------\n", logfile);

        for (int fd: leaks) {
            sprintf(num, "%d", fd);
            ssize_t rc = readlinkat(dd, num, buf, sizeof(buf) - 1);
            if (rc >= 0) {
                buf[rc] = '\0';
                fprintf(logfile, "\t%d: %s\n", fd, buf);
            }

            close(fd);
        }

        fclose(logfile);
    }
#endif

    closedir(dir);
}
