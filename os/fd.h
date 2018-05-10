// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <unordered_map>
#include <dirent.h>

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#define a_sec_field st_atim.tv_sec
#define a_nsec_field st_atim.tv_nsec
#define m_sec_field st_mtim.tv_sec
#define m_nsec_field st_mtim.tv_nsec
#define c_sec_field st_ctim.tv_sec
#define c_nsec_field st_ctim.tv_nsec
#define osTellDir(dir, ent) ent->d_off
#else
/* Mac OS X */
#define AT_NO_AUTOMOUNT 0
#define a_sec_field st_atimespec.tv_sec
#define a_nsec_field st_atimespec.tv_nsec
#define m_sec_field st_mtimespec.tv_sec
#define m_nsec_field st_mtimespec.tv_nsec
#define c_sec_field st_ctimespec.tv_sec
#define c_nsec_field st_ctimespec.tv_nsec
#define osTellDir(dir, ent) telldir(dir)
#endif

struct FileInfo {
    uint64_t mtime, size;
    uint32_t mode, uid, gid;

    int dirfd;
    std::unordered_map<std::string,std::string> fattr;

    // readdir only
    std::string name;

    inline FileInfo(int fd) : dirfd(fd) {}
};

extern void
osPurgeFileDescriptors(const char *program);

extern int
osOpenFile(const char *name, size_t *sizeret, uint32_t *moderet);

extern int
osOpenDir(const char *path, DIR **dirret);

extern int
osStatFile(const char *name, FileInfo *info);

extern int
osReadDir(DIR *dir, FileInfo *info);

extern bool
osFileExists(const char *path, bool *isdirret = nullptr);

extern bool
osFindDirUpwards(std::string &pathinout, const char *dir);

extern int
osRecursiveDelete(const std::string &path);

extern int
osOpenRenamedFile(std::string &pathret, unsigned mode, bool nofd = false);

extern int
osMkpath(const std::string &path, unsigned mode);
