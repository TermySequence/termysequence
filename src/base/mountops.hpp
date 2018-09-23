// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

// Included into base/mounttask.cpp

#include "os/fd.h"

#include <cerrno>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define MIN_READ 32768
#define MAX_READ 131072
#define MAX_READ_STR "131072"
#define INVALID_FH UINT64_MAX

#define declare_thiz \
    auto *thiz = static_cast<MountTask*>(fuse_req_userdata(req))

MountTask::Inode::Inode(inode_t ino_) :
    isinvalid(false),
    isdir(false),
    isremote(false),
    refcount(1),
    ino(ino_)
{}

inline MountTask::Inode *
MountTask::lookupInode(inode_t ino) const
{
    auto i = m_inodes.find(ino);
    return (i != m_inodes.end() && !i->second->isinvalid) ?
        i->second : nullptr;
}

inline void
MountTask::unrefInode(inode_t ino, unsigned count)
{
    auto i = m_inodes.find(ino);
    if (i != m_inodes.end() && (i->second->refcount -= count) == 0) {
        delete i->second;
        m_inodes.erase(i);
    }
}

void
MountTask::unrefInode(MountTask::Inode *irec)
{
    if (--irec->refcount == 0) {
        m_inodes.erase(irec->ino);
        delete irec;
    }
}

MountTask::Inode *
MountTask::createInode(const std::string &name, int fd)
{
    auto *irec = new MountTask::Inode(++m_nextInode);
    irec->name = name;
    m_rootmap.emplace(irec->name, irec);
    m_inodes.emplace(irec->ino, irec);
    if (fd != -1)
        irec->fds.push_back(fd);
    return irec;
}

MountTask::Inode *
MountTask::addInode(Dirmap &dirmap, const std::string &dir, const std::string &name)
{
    auto i = dirmap.find(name);
    if (i == dirmap.end()) {
        auto *irec = new MountTask::Inode(++m_nextInode);
        irec->isremote = true;
        irec->name = dir + name;
        m_inodes.emplace(irec->ino, irec);
        dirmap.emplace(name, irec);
        return irec;
    } else {
        return i->second;
    }
}

MountTask::Inode *
MountTask::renameFile(const std::string &oldname, const std::string &newname)
{
    auto i = m_rootmap.find(oldname);
    auto irec = i->second;
    irec->name = newname;
    m_rootmap.erase(i);

    i = m_rootmap.find(newname);
    if (i != m_rootmap.end()) {
        i->second->name.clear();
        unrefInode(i->second);
        i->second = irec;
    } else {
        m_rootmap.emplace(newname, irec);
    }

    return irec;
}

inline void
MountTask::unlinkFile(const std::string &name)
{
    auto i = m_rootmap.find(name);
    if (i != m_rootmap.end()) {
        i->second->name.clear();
        unrefInode(i->second);
        m_rootmap.erase(i);
    }
}

char *
MountTask::outbuf(size_t size)
{
    if (size > m_outsize) {
        do {
            m_outsize *= 2;
        } while (size > m_outsize);

        delete [] m_outbuf;
        m_outbuf = new char[m_outsize];
    }
    return m_outbuf;
}

char *
MountTask::outbuf(size_t size, bool)
{
    if (size > m_outsize) {
        size_t oldsize = m_outsize;
        do {
            m_outsize *= 2;
        } while (size > m_outsize);

        char *newbuf = new char[m_outsize];
        memcpy(newbuf, m_outbuf, oldsize);
        delete [] m_outbuf;
        m_outbuf = newbuf;
    }
    return m_outbuf;
}

extern "C" void
mountop_init(void *, struct fuse_conn_info *conn)
{
    conn->max_write = MAX_READ;
#if USE_FUSE3
    conn->max_read = MAX_READ;
#endif
    conn->max_readahead = MAX_READ;

    conn->want &= ~(FUSE_CAP_ASYNC_READ|FUSE_CAP_ATOMIC_O_TRUNC|FUSE_CAP_ASYNC_DIO|
                    FUSE_CAP_PARALLEL_DIROPS|FUSE_CAP_HANDLE_KILLPRIV);
    conn->want |= (FUSE_CAP_SPLICE_WRITE|FUSE_CAP_SPLICE_MOVE|FUSE_CAP_SPLICE_READ|
                   FUSE_CAP_IOCTL_DIR|FUSE_CAP_BIG_WRITES);

    conn->max_background = 1;
    conn->congestion_threshold = 0;
}

static inline bool
populate_attr(Tsq::ProtocolUnmarshaler *unm, MountTask *thiz, struct stat *attr)
{
    unsigned mode = unm->parseNumber();
    attr->st_mode = (S_ISDIR(mode) ? S_IFDIR : S_IFREG) | (mode & 0777);
    attr->st_nlink = 1;
    attr->st_uid = thiz->uid();
    attr->st_gid = thiz->gid();
    attr->st_size = unm->parseNumber64();
    attr->st_atim.tv_sec = unm->parseNumber64();
    attr->st_atim.tv_nsec = unm->parseNumber() % 1000000000;
    attr->st_mtim.tv_sec = unm->parseNumber64();
    attr->st_mtim.tv_nsec = unm->parseNumber() % 1000000000;
    attr->st_ctim.tv_sec = unm->parseNumber64();
    attr->st_ctim.tv_nsec = unm->parseNumber() % 1000000000;
    return S_ISDIR(mode);
}

extern "C" void
mountop_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    declare_thiz;
    auto *p = new struct stat();
    const auto *irec = thiz->lookupInode(ino);

    if (!irec) {
        errno = EIO;
        goto out;
    }
    if (irec->isremote) {
        p->st_ino = irec->ino;
        thiz->prepRequest(Tsq::MountTaskOpStat, irec->name, req, p);
        thiz->pushRequest();
        return;
    }

    errno = 0;
    if (!irec->fds.empty()) {
        fstat(irec->fds.back(), p);
    } else {
        fstatat(thiz->dirfd(), irec->name.c_str(), p, 0);
    }
out:
    if (errno) {
        fuse_reply_err(req, errno);
    } else {
        p->st_ino = ino;
        fuse_reply_attr(req, p, FUSE_LOCAL_VALIDITY_PERIOD);
    }

    delete p;
}

static void
mount_getattr_result(Tsq::ProtocolUnmarshaler *unm, fuse_req_t req, void *buf)
{
    switch (unm->parseNumber()) {
    case Tsq::MountTaskSuccess:
        break;
    case Tsq::MountTaskExist:
        fuse_reply_err(req, ENOENT);
        return;
    default:
        fuse_reply_err(req, EIO);
        return;
    }

    declare_thiz;
    auto *p = static_cast<struct stat*>(buf);
    auto irec = thiz->lookupInode(p->st_ino);
    if (!irec) {
        fuse_reply_err(req, EIO);
        return;
    }

    irec->isdir = populate_attr(unm, thiz, p);
    fuse_reply_attr(req, p, FUSE_REMOTE_VALIDITY_PERIOD);
}

extern "C" void
mountop_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                int to_set, struct fuse_file_info *fi)
{
    declare_thiz;

    if (ino == FUSE_ROOT_ID) {
        // do nothing
        mountop_getattr(req, ino, fi);
        return;
    }

    const auto *irec = thiz->lookupInode(ino);
    const char *namec;

    if (!irec) {
        errno = EIO;
        goto out;
    }
    if ((to_set & FUSE_SET_ATTR_UID && attr->st_uid != thiz->uid()) ||
        (to_set & FUSE_SET_ATTR_GID && attr->st_gid != thiz->gid())) {
        errno = EPERM;
        goto out;
    }
    if (irec->isremote) {
        auto *p = new struct stat();
        p->st_ino = irec->ino;
        if (to_set & FUSE_SET_ATTR_MODE) {
            thiz->prepMessage(Tsq::MountTaskOpChmod, irec->name);
            thiz->addNumber(attr->st_mode & 0777);
            thiz->pushRequest();
        }
        if (to_set & FUSE_SET_ATTR_SIZE) {
            thiz->prepMessage(Tsq::MountTaskOpTrunc, irec->name);
            thiz->addNumber64(attr->st_size);
            thiz->pushRequest();
        }
        if (to_set & (FUSE_SET_ATTR_MTIME|FUSE_SET_ATTR_MTIME_NOW|FUSE_SET_ATTR_CTIME)) {
            thiz->prepMessage(Tsq::MountTaskOpTouch, irec->name);
            thiz->pushRequest();
        }
        thiz->prepRequest(Tsq::MountTaskOpStat, irec->name, req, p);
        thiz->pushRequest();
        return;
    }

    namec = irec->name.c_str();
    errno = 0;

    if (to_set & FUSE_SET_ATTR_SIZE) {
        if (!fi || fi->fh == INVALID_FH) {
            int rc = -1;
            int fd = openat(thiz->dirfd(), namec, O_RDWR|O_NOFOLLOW|O_CLOEXEC|O_NOCTTY);
            if (fd >= 0) {
                rc = ftruncate(fd, attr->st_size);
                int errsave = errno;
                close(fd);
                errno = errsave;
            }
            if (rc != 0)
                goto out;
        }
        else if (ftruncate(fi->fh, attr->st_size) != 0)
            goto out;
    }
    if (!irec->fds.empty()) {
        if (to_set & FUSE_SET_ATTR_MODE &&
            fchmod(irec->fds.back(), attr->st_mode & 0777) != 0)
            goto out;

        if (to_set & (FUSE_SET_ATTR_MTIME|FUSE_SET_ATTR_MTIME_NOW|FUSE_SET_ATTR_CTIME) &&
            futimens(irec->fds.back(), NULL) != 0)
            goto out;
    } else {
        if (to_set & FUSE_SET_ATTR_MODE &&
            fchmodat(thiz->dirfd(), namec, attr->st_mode & 0777, 0) != 0)
            goto out;

        if (to_set & (FUSE_SET_ATTR_MTIME|FUSE_SET_ATTR_MTIME_NOW|FUSE_SET_ATTR_CTIME) &&
            utimensat(thiz->dirfd(), namec, NULL, 0) != 0)
            goto out;
    }
out:
    if (errno) {
        fuse_reply_err(req, errno);
    } else {
        mountop_getattr(req, ino, fi);
    }
}

struct LookupReqState {
    std::string dir;
    std::string name;
};

extern "C" void
mountop_lookup(fuse_req_t req, fuse_ino_t parent, const char *namec)
{
    declare_thiz;
    const auto *prec = thiz->lookupInode(parent);
    std::string name(namec);

    if (!prec) {
        fuse_reply_err(req, EIO);
        return;
    }
    if (prec->isremote || thiz->file() == name) {
        auto *state = new LookupReqState;
        state->dir = prec->name;
        state->name = std::move(name);
        std::string path = state->dir + '/' + state->name;
        thiz->prepRequest(Tsq::MountTaskOpLookup, path, req, state);
        thiz->pushRequest();
        return;
    }

    auto i = thiz->rootmap().find(namec);
    if (i == thiz->rootmap().end()) {
        fuse_reply_err(req, ENOENT);
        return;
    }
    auto *irec = i->second;
    struct fuse_entry_param p = {};

    int rc = irec->fds.empty() ?
        fstatat(thiz->dirfd(), namec, &p.attr, 0) :
        fstat(irec->fds.back(), &p.attr);

    if (rc != 0) {
        fuse_reply_err(req, errno);
    } else {
        p.generation = 1;
        p.entry_timeout = FUSE_LOCAL_VALIDITY_PERIOD;
        p.attr_timeout = FUSE_LOCAL_VALIDITY_PERIOD;
        p.ino = p.attr.st_ino = irec->ino;
        ++irec->refcount;
        fuse_reply_entry(req, &p);
    }
}

static void
mount_lookup_result(Tsq::ProtocolUnmarshaler *unm, fuse_req_t req, void *buf)
{
    declare_thiz;
    auto *state = static_cast<LookupReqState*>(buf);

    switch (unm->parseNumber()) {
    case Tsq::MountTaskSuccess:
        break;
    case Tsq::MountTaskExist:
        fuse_reply_err(req, ENOENT);
        return;
    default:
        fuse_reply_err(req, EIO);
        return;
    }

    auto &dirmap = thiz->dirmap(state->dir);
    if (!state->dir.empty())
        state->dir.push_back('/');
    auto *irec = thiz->addInode(dirmap, state->dir, state->name);

    struct fuse_entry_param p = {};
    p.generation = 1;
    p.entry_timeout = FUSE_LOCAL_VALIDITY_PERIOD;
    p.attr_timeout = FUSE_REMOTE_VALIDITY_PERIOD;
    p.ino = p.attr.st_ino = irec->ino;

    irec->isdir = populate_attr(unm, thiz, &p.attr);
    ++irec->refcount;
    fuse_reply_entry(req, &p);
}

extern "C" void
mountop_unlink(fuse_req_t req, fuse_ino_t parent, const char *namec)
{
    declare_thiz;
    std::string name(namec);

    if (thiz->rootmap().count(name) == 0) {
        errno = ENOENT;
        goto out;
    }

    errno = 0;
    if (thiz->file() == name || unlinkat(thiz->dirfd(), namec, 0) == 0) {
        thiz->unlinkFile(name);
    }
out:
    fuse_reply_err(req, errno);
}

extern "C" void
mountop_forget(fuse_req_t req, fuse_ino_t ino,
#if USE_FUSE3
               uint64_t nlookup)
#else
               unsigned long nlookup)
#endif
{
    declare_thiz;
    thiz->unrefInode(ino, nlookup);
    fuse_reply_none(req);
}

extern "C" void
mountop_forget_multi(fuse_req_t req, size_t count, struct fuse_forget_data *a)
{
    declare_thiz;

    for (size_t i = 0; i < count; ++i)
        thiz->unrefInode(a[i].ino, a[i].nlookup);

    fuse_reply_none(req);
}

struct CreateReqState {
    struct fuse_entry_param p;
    struct fuse_file_info fi;
    bool iscreate;
};

static std::pair<MountTask::Inode*,bool>
create_file(fuse_req_t req, fuse_ino_t parent, const char *namec, mode_t mode,
            unsigned flags, struct fuse_entry_param *p)
{
    declare_thiz;
    std::string name(namec);

    p->generation = 1;
    p->entry_timeout = FUSE_LOCAL_VALIDITY_PERIOD;

    if (thiz->rootmap().count(name)) {
        errno = EEXIST;
        return std::make_pair(nullptr, false);
    }
    if (!S_ISREG(mode)) {
        errno = EPERM;
        return std::make_pair(nullptr, false);
    }

    if (thiz->file() == name) {
        p->attr.st_mode = S_IFREG;
        return std::make_pair(nullptr, true);
    }

    flags &= (O_WRONLY|O_RDWR);
    int fd = openat(thiz->dirfd(), namec, flags|O_CREAT|O_EXCL|O_CLOEXEC, mode);
    if (fd < 0)
        return std::make_pair(nullptr, false);

    fstat(fd, &p->attr);

    auto *irec = thiz->createInode(name, fd);
    ++irec->refcount;
    thiz->rootmap().emplace(irec->name, irec);
    p->ino = p->attr.st_ino = irec->ino;
    p->attr_timeout = FUSE_LOCAL_VALIDITY_PERIOD;
    return std::make_pair(irec, false);
}

extern "C" void
mountop_mknod(fuse_req_t req, fuse_ino_t parent, const char *namec,
              mode_t mode, dev_t rdev)
{
    auto *state = new CreateReqState();
    auto ret = create_file(req, parent, namec, mode, O_WRONLY, &state->p);
    if (ret.first) {
        close(ret.first->fds.back());
        ret.first->fds.pop_back();
        fuse_reply_entry(req, &state->p);
        delete state;
    } else if (!ret.second) {
        fuse_reply_err(req, errno);
        delete state;
    } else {
        declare_thiz;
        thiz->prepRequest(Tsq::MountTaskOpCreate, thiz->file(), req, state);
        thiz->addNumber(mode);
        thiz->pushRequest();
    }
}

extern "C" void
mountop_create(fuse_req_t req, fuse_ino_t parent, const char *name,
               mode_t mode, struct fuse_file_info *fi)
{
    declare_thiz;
    auto *state = new CreateReqState();
    auto ret = create_file(req, parent, name, mode, fi->flags, &state->p);
    if (ret.first) {
        fi->fh = ret.first->fds.back();
        fuse_reply_create(req, &state->p, fi);
        thiz->reportOpen();
        delete state;
    } else if (!ret.second) {
        fuse_reply_err(req, errno);
        delete state;
    } else {
        thiz->prepRequest(Tsq::MountTaskOpCreate, thiz->file(), req, state);
        thiz->addNumber(mode);
        thiz->pushRequest();
        memcpy(&state->fi, fi, sizeof(*fi));
        state->iscreate = true;
    }
}

static void
mount_create_result(Tsq::ProtocolUnmarshaler *unm, fuse_req_t req, void *buf)
{
    declare_thiz;
    auto *state = static_cast<CreateReqState*>(buf);

    // Check for remote write error
    if (unm->parseNumber()) {
        fuse_reply_err(req, EIO);
        return;
    }
    if (populate_attr(unm, thiz, &state->p.attr)) {
        fuse_reply_err(req, EIO);
        return;
    }

    auto *irec = thiz->createInode(thiz->file());
    irec->isremote = true;
    ++irec->refcount;
    thiz->rootmap().emplace(irec->name, irec);
    state->p.ino = state->p.attr.st_ino = irec->ino;
    state->p.attr_timeout = FUSE_REMOTE_VALIDITY_PERIOD;

    if (state->iscreate) {
        fuse_reply_create(req, &state->p, &state->fi);
        thiz->reportOpen();
    } else {
        fuse_reply_entry(req, &state->p);
    }
}

struct RenameReqState {
    int fd;
    size_t size;
    size_t off;
    size_t last;
    MountTask::Inode *irec;
};

static void
perform_replace(fuse_req_t req, std::string &oldname, const char *oldnamec)
{
    declare_thiz;

    // Open a handle to the local file
    int fd = openat(thiz->dirfd(), oldnamec, O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NOCTTY);
    if (fd < 0) {
        fuse_reply_err(req, errno);
        return;
    }

    // Start reading
    size_t size = MIN_READ;
    char *outbuf = thiz->outbuf(size);
    ssize_t rc = read(fd, outbuf, size);
    if (rc < 0) {
        fuse_reply_err(req, errno);
        close(fd);
        return;
    }

    // Unlink the local file
    if (unlinkat(thiz->dirfd(), oldnamec, 0) != 0) {
        fuse_reply_err(req, errno);
        return;
    }

    // At this point, we're definitely writing over the remote file

    // Invalidate any existing inode before the rename
    auto newi = thiz->rootmap().find(thiz->file());
    if (newi != thiz->rootmap().end()) {
        newi->second->isinvalid = true;
    }
    // Swap the inode in
    auto *irec = thiz->renameFile(oldname, thiz->file());
    for (int fd: irec->fds)
        close(fd);
    irec->fds.clear();
    irec->isremote = true;

    // Truncate the remote file
    thiz->prepMessage(Tsq::MountTaskOpTrunc, thiz->file());
    thiz->addNumber64(0);
    thiz->pushRequest();

    if (rc == 0) {
        fuse_reply_err(req, 0);
        close(fd);
        return;
    }

    // Use irec to store the fd in case an unmount happens
    irec->fds.assign(1, fd);

    auto *state = new RenameReqState();
    thiz->prepRequest(Tsq::MountTaskOpUpload, thiz->file(), req, state);
    thiz->addNumber64(0);
    thiz->pushRequest(outbuf, rc);
    thiz->reportSent(rc);
    state->fd = fd;
    state->irec = irec;
    state->size = size;
    state->off = rc;
    state->last = rc;

    // Stop incoming requests
    thiz->setRenaming(true);
}

static bool
mount_replace_result(Tsq::ProtocolUnmarshaler *unm, fuse_req_t req, void *buf)
{
    declare_thiz;
    auto *state = static_cast<RenameReqState*>(buf);
    char *outbuf;
    ssize_t rc;

    // Check for remote write error
    if (unm->parseNumber()) {
        errno = EIO;
        goto finished;
    }
    // Keep reading
    if (state->last == state->size && state->size < MAX_READ)
        state->size *= 2;

    outbuf = thiz->outbuf(state->size);
    errno = 0;
    rc = read(state->fd, outbuf, state->size);
    if (rc <= 0)
        goto finished;

    thiz->prepRequest(Tsq::MountTaskOpUpload, thiz->file(), req, state);
    thiz->addNumber64(state->off);
    thiz->pushRequest(outbuf, rc);
    thiz->reportSent(rc);
    state->off += rc;
    state->last = rc;
    return false;
finished:
    // Close out request
    fuse_reply_err(req, errno);
    // Clean up
    state->irec->fds.clear();
    close(state->fd);
    // Resume incoming requests
    thiz->setRenaming(false);
    return true;
}

static void
perform_copy(fuse_req_t req, std::string &newname, const char *newnamec)
{
    declare_thiz;

    // Remove any existing local file
    if (unlinkat(thiz->dirfd(), newnamec, 0) == 0)
        thiz->unlinkFile(newname);

    // Open a handle to the new local file
    int fd = openat(thiz->dirfd(), newnamec,
                    O_WRONLY|O_CREAT|O_TRUNC|O_NOFOLLOW|O_CLOEXEC|O_NOCTTY,
                    0666);
    if (fd < 0) {
        fuse_reply_err(req, errno);
        return;
    }

    // Swap the inode out
    auto *irec = thiz->renameFile(thiz->file(), newname);
    irec->isremote = false;
    // Use irec to store the fd in case an unmount happens
    irec->fds.assign(1, fd);

    auto *state = new RenameReqState();
    thiz->prepRequest(Tsq::MountTaskOpDownload, thiz->file(), req, state);
    thiz->addNumber64(MAX_READ);
    thiz->addNumber64(0);
    thiz->pushRequest();
    state->fd = fd;
    state->irec = irec;
    state->off = 0;

    // Stop incoming requests
    thiz->setRenaming(true);
}

static bool
mount_copy_result(Tsq::ProtocolUnmarshaler *unm, fuse_req_t req, void *buf)
{
    declare_thiz;
    auto *state = static_cast<RenameReqState*>(buf);
    const char *outbuf;
    size_t sent = 0, total;

    // Check for remote read error
    if (unm->parseNumber()) {
        errno = EIO;
        goto finished;
    }
    // Check for remote EOF
    total = unm->remainingLength();
    if (total == 0)
        goto finished;

    // Keep writing
    outbuf = unm->remainingBytes();
    errno = 0;
    while (sent < total) {
        ssize_t rc = write(state->fd, outbuf, total - sent);
        if (rc < 0)
            goto finished;

        sent += rc;
        outbuf += rc;
    }

    thiz->prepRequest(Tsq::MountTaskOpDownload, thiz->file(), req, state);
    thiz->addNumber64(MAX_READ);
    thiz->addNumber64(state->off += total);
    thiz->pushRequest();
    thiz->reportReceived(total);
    return false;
finished:
    // Close out request
    fuse_reply_err(req, errno);
    // Clean up
    state->irec->fds.erase(state->irec->fds.begin());
    close(state->fd);
    // Resume incoming requests
    thiz->setRenaming(false);
    return true;
}

extern "C" void
mountop_rename(fuse_req_t req, fuse_ino_t, const char *oldnamec,
               fuse_ino_t, const char *newnamec
#if USE_FUSE3
               , unsigned int flags
#endif
    )
{
    declare_thiz;
    std::string oldname(oldnamec), newname(newnamec);

#if USE_FUSE3
    if (flags) {
        fuse_reply_err(req, EINVAL);
        return;
    }
#endif
    if (thiz->rootmap().count(oldname) == 0) {
        fuse_reply_err(req, ENOENT);
        return;
    }
    if (thiz->file() == oldname) {
        perform_copy(req, newname, newnamec);
        return;
    }
    if (thiz->file() == newname) {
        perform_replace(req, oldname, oldnamec);
        return;
    }

    errno = 0;
    if (renameat(thiz->dirfd(), oldnamec, thiz->dirfd(), newnamec) == 0) {
        thiz->renameFile(oldname, newname);
    }
    fuse_reply_err(req, errno);
}

static bool
open_file(fuse_req_t req, MountTask::Inode *irec, struct fuse_file_info *fi)
{
    declare_thiz;
    const char *namec = irec->name.c_str();
    unsigned flags = fi->flags & (O_RDONLY|O_WRONLY|O_RDWR);
    int fd = openat(thiz->dirfd(), namec, flags|O_NOFOLLOW|O_CLOEXEC|O_NOCTTY);
    if (fd < 0) {
        fuse_reply_err(req, errno);
        return false;
    } else {
        fi->fh = fd;
        irec->fds.push_back(fd);
        return true;
    }
}

extern "C" void
mountop_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    declare_thiz;
    auto *irec = thiz->lookupInode(ino);

    if (!irec) {
        fuse_reply_err(req, EIO);
        return;
    }
    if (!irec->isremote) {
        if (open_file(req, irec, fi)) {
            fuse_reply_open(req, fi);
            thiz->reportOpen();
        }
        return;
    }

    fi->fh = INVALID_FH;
    auto *p = new struct fuse_file_info(*fi);

    thiz->prepRequest(Tsq::MountTaskOpOpen, irec->name, req, p);
    thiz->pushRequest();
}

static void
mount_open_result(Tsq::ProtocolUnmarshaler *unm, fuse_req_t req, void *buf)
{
    declare_thiz;
    switch (unm->parseNumber()) {
    case Tsq::MountTaskSuccess:
        fuse_reply_open(req, static_cast<struct fuse_file_info*>(buf));
        thiz->reportOpen();
        break;
    case Tsq::MountTaskExist:
        fuse_reply_err(req, ENOENT);
        break;
    case Tsq::MountTaskFiletype:
        fuse_reply_err(req, EISDIR);
        break;
    default:
        fuse_reply_err(req, EIO);
        break;
    }
}

extern "C" void
mountop_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    declare_thiz;
    auto *irec = thiz->lookupInode(ino);

    if (!irec) {
        errno = EIO;
        goto out;
    }
    if (irec->isremote) {
        thiz->prepMessage(Tsq::MountTaskOpClose, irec->name);
        thiz->pushRequest();
        errno = 0;
        goto out;
    }

    errno = 0;
    if (fi->fh != INVALID_FH) {
        for (auto i = irec->fds.begin(), j = irec->fds.end(); i != j; ++i) {
            if (*i == (int)fi->fh) {
                irec->fds.erase(i);
                break;
            }
        }
        close(fi->fh);
    }
out:
    fuse_reply_err(req, errno);
    thiz->reportClose();
}

extern "C" void
mountop_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
             struct fuse_file_info *fi)
{
    declare_thiz;
    auto *irec = thiz->lookupInode(ino);

    if (!irec) {
        fuse_reply_err(req, EIO);
        return;
    }
    if (!irec->isremote) {
        if (fi->fh == INVALID_FH && !open_file(req, irec, fi))
            return;

        struct fuse_bufvec bv = { 1 };
        bv.buf[0].size = size;
        bv.buf[0].flags = (fuse_buf_flags)(FUSE_BUF_IS_FD|FUSE_BUF_FD_SEEK|FUSE_BUF_FD_RETRY);
        bv.buf[0].fd = fi->fh;
        bv.buf[0].pos = off;
        fuse_reply_data(req, &bv, FUSE_BUF_SPLICE_MOVE);
        return;
    }

    auto *p = new size_t(size);

    thiz->prepRequest(Tsq::MountTaskOpRead, irec->name, req, p);
    thiz->addNumber64(size);
    thiz->addNumber64(off);
    thiz->pushRequest();
}

static void
mount_read_result(Tsq::ProtocolUnmarshaler *unm, fuse_req_t req, void *buf)
{
    switch (unm->parseNumber()) {
    case Tsq::MountTaskSuccess:
        break;
    case Tsq::MountTaskFiletype:
        fuse_reply_err(req, EISDIR);
        return;
    default:
        fuse_reply_err(req, EIO);
        return;
    }

    declare_thiz;
    size_t origsize = *static_cast<size_t*>(buf);

    if (unm->remainingLength() > origsize) {
        fuse_reply_err(req, EIO);
    } else {
        fuse_reply_buf(req, unm->remainingBytes(), unm->remainingLength());
        thiz->reportReceived(unm->remainingLength());
    }
}

extern "C" void
mountop_write(fuse_req_t req, fuse_ino_t ino, struct fuse_bufvec *bv, off_t off,
              struct fuse_file_info *fi)
{
    declare_thiz;
    auto *irec = thiz->lookupInode(ino);
    size_t size = bv->buf[0].size, sent = 0;
    ssize_t rc;
    char *outbuf;
    auto op = (fi->flags & O_APPEND) ? Tsq::MountTaskOpAppend : Tsq::MountTaskOpWrite;

    if (!irec) {
        fuse_reply_err(req, EIO);
        return;
    }
    if (!irec->isremote) {
        if (fi->fh == INVALID_FH && !open_file(req, irec, fi))
            return;

        if (bv->buf[0].flags & FUSE_BUF_IS_FD) {
            while (sent < size) {
                rc = splice(bv->buf[0].fd, NULL, fi->fh, &off, size - sent, SPLICE_F_MOVE);
                if (rc < 0) {
                    fuse_reply_err(req, errno);
                    goto out;
                }
                if (rc == 0) {
                    break;
                }

                sent += rc;
            }
        } else {
            while (sent < size) {
                const char *ptr = static_cast<char*>(bv->buf[0].mem);
                lseek(fi->fh, off, SEEK_SET);
                rc = write(fi->fh, ptr, size - sent);
                if (rc < 0) {
                    fuse_reply_err(req, errno);
                    goto out;
                }

                sent += rc;
                ptr += rc;
            }
        }
        fuse_reply_write(req, sent);
        goto out;
    }

    if (bv->buf[0].flags & FUSE_BUF_IS_FD) {
        outbuf = thiz->outbuf(size);
        while (sent < size) {
            rc = read(bv->buf[0].fd, outbuf + sent, size - sent);
            if (rc < 0) {
                fuse_reply_err(req, errno);
                goto out;
            }
            if (rc == 0)
                break;

            sent += rc;
        }
    } else {
        outbuf = static_cast<char*>(bv->buf[0].mem);
        sent = size;
    }

    thiz->prepRequest(op, irec->name, req);
    thiz->addNumber64(off);
    thiz->pushRequest(outbuf, sent);
    thiz->reportSent(sent);
out:
    bv->off += sent;
}

static void
mount_write_result(Tsq::ProtocolUnmarshaler *unm, fuse_req_t req)
{
    if (unm->parseNumber())
        fuse_reply_err(req, EIO);
    else
        fuse_reply_write(req, unm->parseNumber64());
}

extern "C" void
mountop_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    if (ino == FUSE_ROOT_ID) {
        fuse_reply_open(req, fi);
        return;
    }

    declare_thiz;
    auto *irec = thiz->lookupInode(ino);

    if (!irec) {
        fuse_reply_err(req, EIO);
        return;
    }
    if (!irec->isdir) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    auto *p = new struct fuse_file_info(*fi);
    thiz->prepRequest(Tsq::MountTaskOpOpendir, irec->name, req, p);
    thiz->pushRequest();
}

static void
mount_opendir_result(Tsq::ProtocolUnmarshaler *unm, fuse_req_t req, void *buf)
{
    declare_thiz;
    auto *fi = static_cast<struct fuse_file_info*>(buf);

    switch (unm->parseNumber()) {
    case Tsq::MountTaskSuccess:
        fi->fh = (uint64_t)thiz->createSeenset();
        fuse_reply_open(req, fi);
        break;
    case Tsq::MountTaskExist:
        fuse_reply_err(req, ENOENT);
        break;
    case Tsq::MountTaskFiletype:
        fuse_reply_err(req, ENOTDIR);
        break;
    default:
        fuse_reply_err(req, EIO);
        break;
    }
}

struct ReaddirReqState {
    std::string dir;
    size_t origsize;
    MountTask::Seenset *seenset;
};

extern "C" void
mountop_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                off_t off, struct fuse_file_info *fi)
{
    declare_thiz;

    if (ino != FUSE_ROOT_ID) {
        auto *irec = thiz->lookupInode(ino);
        if (!irec) {
            fuse_reply_err(req, EIO);
            return;
        }

        auto *state = new ReaddirReqState();
        state->dir = irec->name;
        state->origsize = size;
        state->seenset = (MountTask::Seenset*)fi->fh;

        thiz->prepRequest(Tsq::MountTaskOpReaddir, irec->name, req, state);
        thiz->addNumber64(size);
        thiz->addNumber64(off);
        thiz->addNumber(8 + fuse_add_direntry(req, 0, 0, "", 0, 0));
        thiz->pushRequest();
        return;
    }

    struct stat attr = {};
    size_t total = 0, offset = 0;
    char *outbuf = nullptr;

    const auto &dirmap = thiz->rootmap();
    auto i = dirmap.begin(), j = dirmap.end();
    for (off_t n = 0; n < off && i != j; ++i, ++n);

    while (i != j) {
        const char *name = i->first.c_str();
        size_t cursize = fuse_add_direntry(req, 0, 0, name, 0, 0);
        total += cursize;
        if (total > size)
            break;
        outbuf = thiz->outbuf(total, true);

        attr.st_ino = i->second->ino;
        attr.st_mode = i->second->isdir ? S_IFDIR : S_IFREG;
        fuse_add_direntry(req, outbuf + offset, cursize, name, &attr, ++off);

        offset += cursize;
        ++i;
    }

    fuse_reply_buf(req, outbuf, total);
}

static bool
validate_filename(const char *namec)
{
    size_t n = strlen(namec);
    unsigned char *ptr = (unsigned char*)namec;

    if (n == 0 || n > MAXNAME)
        return false;

    for (size_t i = 0; i < n; ++i)
        if (ptr[i] == '/' || ptr[i] == '\x7f' || ptr[i] < '\x20')
            return false;

    if (n > 2)
        return true;

    // Don't report . and .. entries
    return strcmp(namec, ".") && strcmp(namec, "..");
}

static void
mount_readdir_result(Tsq::ProtocolUnmarshaler *unm, fuse_req_t req, void *buf)
{
    declare_thiz;
    auto *state = static_cast<ReaddirReqState*>(buf);
    struct stat attr = {};
    size_t total = 0, offset = 0;
    char *outbuf = nullptr;

    switch (unm->parseNumber()) {
    case Tsq::MountTaskSuccess:
        break;
    case Tsq::MountTaskExist:
        fuse_reply_err(req, ENOENT);
        return;
    default:
        fuse_reply_err(req, EIO);
        return;
    }

    auto &dirmap = thiz->dirmap(state->dir);
    state->dir.push_back('/');

    while (unm->remainingLength()) {
        unsigned mode = unm->parseNumber();
        off_t off = unm->parseNumber64();
        const char *namec = unm->parsePaddedString();
        if (!validate_filename(namec))
            continue;

        size_t cursize = fuse_add_direntry(req, 0, 0, namec, 0, 0);
        total += cursize;
        if (total > state->origsize)
            break;
        outbuf = thiz->outbuf(total, true);

        std::string name(namec);
        auto irec = thiz->addInode(dirmap, state->dir, name);

        irec->isdir = S_ISDIR(mode);
        attr.st_ino = irec->ino;
        attr.st_mode = irec->isdir ? S_IFDIR : S_IFREG;
        fuse_add_direntry(req, outbuf + offset, cursize, namec, &attr, off);
        offset += cursize;

        state->seenset->emplace(std::move(name));
    }
    if (total == 0) {
        // Reached-end marker
        state->seenset->emplace(std::string());
    }

    fuse_reply_buf(req, outbuf, total);
}

extern "C" void
mountop_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
    if (ino == FUSE_ROOT_ID) {
        fuse_reply_err(req, 0);
        return;
    }

    declare_thiz;
    auto *irec = thiz->lookupInode(ino);
    if (!irec) {
        fuse_reply_err(req, EIO);
        return;
    }

    thiz->prepMessage(Tsq::MountTaskOpClosedir, irec->name);
    thiz->pushRequest();

    auto *seenset = (MountTask::Seenset*)fi->fh;
    if (seenset->count(std::string()))
    {
        auto &dirmap = thiz->dirmap(irec->name);
        for (auto i = dirmap.begin(); i != dirmap.end(); ) {
            if (seenset->count(i->first)) {
                ++i;
            } else {
                i->second->name.clear();
                i->second->isinvalid = true;
                thiz->unrefInode(i->second);
                i = dirmap.erase(i);
            }
        }
    }

    thiz->destroySeenset(seenset);
    fuse_reply_err(req, 0);
}

extern "C" void
mountop_mkdir(fuse_req_t req, fuse_ino_t, const char *, mode_t)
{
    fuse_reply_err(req, EPERM);
}

extern "C" void
mountop_symlink(fuse_req_t req, const char *, fuse_ino_t, const char *)
{
    fuse_reply_err(req, EPERM);
}

extern "C" void
mountop_link(fuse_req_t req, fuse_ino_t, fuse_ino_t, const char *)
{
    fuse_reply_err(req, EPERM);
}

static void
mount_cleanup(Tsq::MountTaskOpcode op, void *buf)
{
    switch (op) {
    case Tsq::MountTaskOpLookup:
        delete static_cast<LookupReqState*>(buf);
        break;
    case Tsq::MountTaskOpStat:
        delete static_cast<struct stat*>(buf);
        break;
    case Tsq::MountTaskOpRead:
        delete static_cast<size_t*>(buf);
        break;
    case Tsq::MountTaskOpUpload:
    case Tsq::MountTaskOpDownload:
        delete static_cast<RenameReqState*>(buf);
        break;
    case Tsq::MountTaskOpOpen:
    case Tsq::MountTaskOpOpendir:
        delete static_cast<struct fuse_file_info*>(buf);
        break;
    case Tsq::MountTaskOpReaddir:
        delete static_cast<ReaddirReqState*>(buf);
        break;
    case Tsq::MountTaskOpCreate:
        delete static_cast<CreateReqState*>(buf);
        break;
    default:
        break;
    }
}

static const char *s_mountargv[] = {
    APP_NAME,
    // "-o", "debug",
    "-o", "noatime",
    "-o", "default_permissions",
    "-o", "max_read=" MAX_READ_STR,
    // This must be last
    "-o", "ro"
};
