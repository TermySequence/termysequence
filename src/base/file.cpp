// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "file.h"
#include "settings/dircolors.h"
#include "lib/wire.h"
#include "lib/attr.h"
#include "lib/exception.h"

#include <QDateTime>

TermDirectory::TermDirectory()
{
    overlimit = false;
    iserror = false;
}

TermDirectory::TermDirectory(Tsq::ProtocolUnmarshaler *unm)
{
    now = unm->parseNumber64();
    name = QString::fromStdString(unm->parseString());
    overlimit = false;
    iserror = false;

    while (unm->remainingLength()) {
        std::string key = unm->parseString();

        if (key.empty())
            break;

        if (key == TSQ_ATTR_FILE_OVERLIMIT) {
            limit = QString::fromStdString(unm->parseString()).toUInt();
            overlimit = true;
            iserror = true;
        }
        else if (key == TSQ_ATTR_FILE_ERROR) {
            error = QString::fromStdString(unm->parseString());
            iserror = true;
        }
        else {
            attributes.insert(QString::fromStdString(key),
                              QString::fromStdString(unm->parseString()));
        }
    }

    if (!name.startsWith('/') && !iserror)
        throw Tsq::ProtocolException(ENOTDIR);
}

unsigned
TermFile::compareTo(const TermFile &other) const
{
    unsigned result;
    result = (mtime != other.mtime) ? FcMtime : 0;
    result |= (size != other.size) ? FcSize : 0;
    result |= ((mode & 0170000) != (other.mode & 0170000)) ? FcType : 0;
    result |= ((mode & 07777) != (other.mode & 07777)) ? FcPerms : 0;
    result |= (uid != other.uid) ? FcUser : 0;
    result |= (gid != other.gid) ? FcGroup : 0;
    result |= (gitflags != other.gitflags) ? FcGit : 0;

    if (linkmode != other.linkmode || link != other.link)
        result |= FcOther;

    return result;
}

static inline char
getClassifier(uint32_t mode)
{
    switch (mode & 0170000) {
    case 0040000:
        return '/';
    case 0120000:
        return '@';
    case 0010000:
        return '|';
    case 0140000:
        return '=';
    default:
        return (mode & 0111) ? '*' : '\0';
    }
}

static QString
getModeString(uint32_t mode)
{
    QString result;
    char modechar;

    switch (mode & 0170000) {
    case 0060000:
        modechar = 'b';
        break;
    case 0020000:
        modechar = 'c';
        break;
    case 0040000:
        modechar = 'd';
        break;
    case 0120000:
        modechar = 'l';
        break;
    case 0010000:
        modechar = 'p';
        break;
    case 0140000:
        modechar = 's';
        break;
    default:
        modechar = '-';
        break;
    }

    result.append(modechar);

    uint32_t tmp = mode;
    for (int i = 0; i < 3; ++i) {
        result.append((tmp & 0400) ? 'r' : '-');
        result.append((tmp & 0200) ? 'w' : '-');
        result.append((tmp & 0100) ? 'x' : '-');
        tmp <<= 3;
    }

    if (mode & 04000)
        result[6] = 's';
    if (mode & 02000)
        result[3] = 's';
    if (mode & 01000)
        result[0] = 't';

    return result;
}

inline unsigned char
TermFile::getLinkCategory()
{
    if (!islink)
        return Dircolors::None;
    if (isorphan)
        return Dircolors::Missing;

    unsigned char result;

    switch (linkmode & 0170000) {
    case 0060000:
        result = Dircolors::Blk;
        break;
    case 0020000:
        result = Dircolors::Chr;
        break;
    case 0040000:
        result = Dircolors::Dir;
        break;
    case 0010000:
        result = Dircolors::Fifo;
        break;
    case 0140000:
        result = Dircolors::Sock;
        break;
    case 0100000:
        result = Dircolors::File;
        if (mode & 02)
            result = Dircolors::OtherW;
        if (mode & 0111)
            result = Dircolors::Exec;
        break;
    default:
        result = Dircolors::None;
        if (mode & 02)
            result = Dircolors::OtherW;
        if (mode & 0111)
            result = Dircolors::Exec;
        break;
    }

    if (linkmode & 0111)
        result = Dircolors::Exec;
    if (linkmode & 01000)
        result = Dircolors::Sticky;
    if ((linkmode & 01002) == 01002)
        result = Dircolors::StickyOtherW;
    if (linkmode & 02000)
        result = Dircolors::SetGID;
    if (linkmode & 04000)
        result = Dircolors::SetUID;

    return result;
}

inline unsigned char
TermFile::getFileCategory()
{
    if (isorphan)
        return Dircolors::Orphan;

    unsigned char result;

    switch (mode & 0170000) {
    case 0060000:
        result = Dircolors::Blk;
        break;
    case 0020000:
        result = Dircolors::Chr;
        break;
    case 0040000:
        result = Dircolors::Dir;
        break;
    case 0120000:
        result = Dircolors::Link;
        break;
    case 0010000:
        result = Dircolors::Fifo;
        break;
    case 0140000:
        result = Dircolors::Sock;
        break;
    case 0100000:
        result = Dircolors::File;
        if (mode & 02)
            result = Dircolors::OtherW;
        if (mode & 0111)
            result = Dircolors::Exec;
        break;
    default:
        result = Dircolors::None;
        if (mode & 02)
            result = Dircolors::OtherW;
        if (mode & 0111)
            result = Dircolors::Exec;
        break;
    }

    if (mode & 01000)
        result = Dircolors::Sticky;
    if ((mode & 01002) == 01002)
        result = Dircolors::StickyOtherW;
    if (mode & 02000)
        result = Dircolors::SetGID;
    if (mode & 04000)
        result = Dircolors::SetUID;

    return result;
}

inline unsigned char
TermFile::getCountIndex()
{
    if ((mode & 0170000) == 0040000)
        return name.startsWith('.') ? FnHiddenDirs : FnDirs;
    else
        return name.startsWith('.') ? FnHiddenFiles : FnFiles;
}

TermFile::TermFile(const TermDirectory &dir, Tsq::ProtocolUnmarshaler *unm)
{
    mtime = unm->parseNumber64();
    size = unm->parseNumber64();
    mode = unm->parseNumber();
    uid = unm->parseNumber();
    gid = unm->parseNumber();
    name = QString::fromStdString(unm->parseString());

    linkmode = 0;
    linkclass = '\0';
    islink = false;
    isorphan = false;
    isgit = false;
    gitflags = 0;

    // Fill in derived fields
    if (name.contains('/'))
        throw Tsq::ProtocolException(EISDIR);
    int idx = name.lastIndexOf('.');
    if (idx != -1)
        ext = name.mid(idx + 1);

    QDateTime date = QDateTime::fromMSecsSinceEpoch(mtime);
    int64_t timediff = dir.now - mtime;
    QString fmt = (timediff < 15552000000l) ? L("MMM dd HH:mm") : L("MMM dd  yyyy");
    mtimestr = date.toString(fmt);
    modestr = getModeString(mode);
    fileclass = getClassifier(mode);
    countindex = getCountIndex();

    // Fill in fields from attributes
    while (unm->remainingLength()) {
        std::string key = unm->parseString();
        if (key.empty())
            break;

        QString value = QString::fromStdString(unm->parseString());

        if (key == TSQ_ATTR_FILE_USER) {
            user = std::move(value);
        }
        else if (key == TSQ_ATTR_FILE_GROUP) {
            group = std::move(value);
        }
        else if (key == TSQ_ATTR_FILE_GIT) {
            isgit = true;
            gitflags = value.toUInt(NULL, 16);
        }
        else if (key == TSQ_ATTR_FILE_LINK) {
            islink = true;
            link = std::move(value);
            linkmode |= 0x80000000;
        }
        else if (key == TSQ_ATTR_FILE_LINKMODE) {
            linkmode |= value.toUInt(NULL, 8) & 0177777;
            linkclass = getClassifier(linkmode);
        }
        else if (key == TSQ_ATTR_FILE_ORPHAN) {
            isorphan = true;
            linkmode |= 0x40000000;
        }
        // TSQ_ATTR_FILE_ERROR ignored for now
    }

    // Use all information to assign color categories
    linkcategory = getLinkCategory();
    category = getFileCategory();
}

TermFile::TermFile()
{
}
