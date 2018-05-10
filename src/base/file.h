// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"
#include "cell.h"

#include <QRgb>

namespace Tsq { class ProtocolUnmarshaler; }

struct TermDirectory
{
    QString name;
    QString error;
    uint64_t now;
    uint32_t limit;
    bool overlimit;
    bool iserror;

    AttributeMap attributes;

    TermDirectory();
    TermDirectory(Tsq::ProtocolUnmarshaler *unm);

    bool sameAs(const TermDirectory &other) const;
};

struct TermFile
{
    enum FileChangeType {
        FcType  = 1,
        FcPerms = 2,
        FcUser  = 4,
        FcGroup = 8,
        FcSize  = 16,
        FcMtime = 32,
        FcGit   = 64,
        FcOther = 128,
    };
    enum FileCountIndex {
        FnDirs,
        FnHiddenDirs,
        FnFiles,
        FnHiddenFiles,
        NFileCounts
    };

    QString name;
    QString ext;
    QString mtimestr;
    QString modestr;
    QString user;
    QString group;

    // Primary fields, all others derived
    QString link;
    uint64_t mtime;
    uint64_t size;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    Tsq::GitStatusFlags gitflags;
    uint32_t linkmode;
    // End primary fields

    unsigned char category;
    char fileclass;
    unsigned char linkcategory;
    char linkclass;

    CellAttributes fattr;
    CellAttributes lattr;

    bool classify;
    bool gittify;
    bool islink;
    bool isorphan;
    bool isgit;
    unsigned char countindex;

    TermFile(const TermDirectory &dir, Tsq::ProtocolUnmarshaler *unm);
    TermFile();

    unsigned compareTo(const TermFile &other) const;
    bool isDir() const;
    void setIsDir(bool isDir);

private:
    unsigned char getLinkCategory();
    unsigned char getFileCategory();
    unsigned char getCountIndex();
};

inline bool
TermDirectory::sameAs(const TermDirectory &other) const
{
    return name == other.name && !(iserror || other.iserror);
}

inline bool
TermFile::isDir() const
{
    return mode & 040000;
}

inline void
TermFile::setIsDir(bool isDir)
{
    mode = isDir ? 040000 : 0;
}
