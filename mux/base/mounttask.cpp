// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "mounttask.h"
#include "listener.h"
#include "exception.h"
#include "os/fd.h"
#include "os/logging.h"
#include "lib/wire.h"
#include "lib/protocol.h"
#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#define HEADERSIZE 60
#define COMMONSIZE 68
#define INITIALSIZE (4096 + COMMONSIZE)

FileMount::FileMount(Tsq::ProtocolUnmarshaler *unm, bool ro) :
    TaskBase("mount", unm, TaskBaseThrottlable | (ro ? 0 : TaskBaseExclusive)),
    m_bufsize(INITIALSIZE),
    m_dir(nullptr),
    m_ro(ro),
    m_isdir(false)
{
    m_targetName = unm->parseString();
    m_timeout = -1;

    uint32_t command = htole32(TSQ_TASK_OUTPUT);
    uint32_t status = htole32(Tsq::TaskRunning);

    m_buf = new char[INITIALSIZE];
    memcpy(m_buf, &command, 4);
    memcpy(m_buf + 8, m_clientId.buf, 16);
    memcpy(m_buf + 24, g_listener->id().buf, 16);
    memcpy(m_buf + 40, m_taskId.buf, 16);
    memcpy(m_buf + 56, &status, 4);
}

FileMount::~FileMount()
{
    delete [] m_buf;
}

/*
 * This thread
 */
void
FileMount::setBufferSize(size_t total)
{
    if (m_bufsize < total) {
        char *newbuf = new char[total + COMMONSIZE];
        memcpy(newbuf, m_buf, HEADERSIZE);
        delete [] m_buf;
        m_buf = newbuf;
        m_ptr = m_buf + COMMONSIZE;
        m_bufsize = total;
    }
}

void
FileMount::handleRequest(std::string *data)
{
    {
        Lock lock(this);
        m_incomingData.erase(data);
    }

    Tsq::ProtocolUnmarshaler unm(data->data(), data->size());
    Req req;
    req.id = unm.parseNumber();
    req.op = (Tsq::MountTaskOpcode)unm.parseNumber();
    req.name = unm.parsePaddedString();

    data->erase(0, unm.currentPosition());
    req.data = std::move(*data);
    delete data;

    m_reqs.push(req);

    if (!m_throttled)
        m_timeout = 0;
}

void
FileMount::addNumber(uint32_t value)
{
    value = htole32(value);
    memcpy(m_ptr, &value, 4);
    m_ptr += 4;
}

void
FileMount::addNumber64(uint64_t value)
{
    value = htole64(value);
    memcpy(m_ptr, &value, 8);
    m_ptr += 8;
}

void
FileMount::addPaddedString(const char *buf, uint32_t len)
{
    memcpy(m_ptr, buf, len);
    m_ptr += len;
    unsigned padding = 4 - ((m_ptr - m_buf) & 3);
    memset(m_ptr, 0, padding);
    m_ptr += padding;
}

void
FileMount::pushReply(unsigned reqid, unsigned reqcode)
{
    unsigned len = m_ptr - m_buf;
    uint32_t val = htole32(len - 8);
    memcpy(m_buf + 4, &val, 4);

    val = htole32(reqid);
    memcpy(m_buf + HEADERSIZE, &val, 4);
    val = htole32(reqcode);
    memcpy(m_buf + HEADERSIZE + 4, &val, 4);

    std::string buf(m_buf, len);

    if (!throttledOutput(buf)) {
        LOGDBG("Mount %p: throttled (local)\n", this);
        m_throttled = true;
        m_timeout = -1;
    }
}

bool
FileMount::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case TaskClose:
        LOGDBG("Mount %p: canceled (code %td)\n", this, item.value);
        return false;
    case TaskInput:
        handleRequest((std::string *)item.value);
        break;
    case TaskPause:
        m_throttled = true;
        m_timeout = -1;
        LOGDBG("Mount %p: throttled (remote)\n", this);
        break;
    case TaskResume:
        m_throttled = false;
        m_timeout = 0;
        LOGDBG("Mount %p: resumed\n", this);
        break;
    default:
        break;
    }

    return true;
}

void
FileMount::handleStat(Req &req)
{
    struct stat info;
    auto ret = Tsq::MountTaskSuccess;
    int rc, fd = -1;
    decltype(m_files)::iterator i;
    decltype(m_dirs)::iterator j;

    // LOGDBG("Mount %p: stat '%s'\n", this, req.name.c_str());

    if ((i = m_files.find(req.name)) != m_files.end())
        fd = i->second.first;
    else if ((j = m_dirs.find(req.name)) != m_dirs.end())
        fd = dirfd(j->second.first);

    rc = (fd >= 0) ?
        fstat(fd, &info) :
        fstatat(m_fd, req.name.c_str(), &info, AT_SYMLINK_NOFOLLOW|AT_NO_AUTOMOUNT);

    if (rc == 0) {
        addNumber(info.st_mode);
        addNumber64(info.st_size);
        addNumber64(info.a_sec_field);
        addNumber(info.a_nsec_field);
        addNumber64(info.m_sec_field);
        addNumber(info.m_nsec_field);
        addNumber64(info.c_sec_field);
        addNumber(info.c_nsec_field);
    } else if (errno == ENOENT) {
        ret = Tsq::MountTaskExist;
    } else {
        ret = Tsq::MountTaskFailure;
    }

    pushReply(req.id, ret);
}

int
FileMount::openSubfd(const char *name, Tsq::MountTaskResult &ret)
{
    // LOGDBG("Mount %p: opening handle to file '%s'\n", this, name);

    int fd = openat(m_fd, name, O_RDONLY|O_CLOEXEC|O_NOCTTY);
    if (fd < 0) {
        ret = (errno == ENOENT) ? Tsq::MountTaskExist : Tsq::MountTaskFailure;
    }
    return fd;
}

void
FileMount::handleRead(Req &req, Tsq::ProtocolUnmarshaler *unm)
{
    size_t total = unm->parseNumber64();
    size_t offset = unm->parseNumber64();
    size_t sent = 0;
    int fd;
    auto ret = Tsq::MountTaskSuccess;
    bool needclose = false;

    // LOGDBG("Mount %p: read '%s': %lu at offset %lu\n", this, req.name.c_str(), total, offset);

    setBufferSize(total);

    auto i = m_files.find(req.name);
    if (i != m_files.end()) {
        fd = i->second.first;
    } else if ((fd = openSubfd(req.name.c_str(), ret)) >= 0) {
        needclose = true;
    } else {
        goto out;
    }

    lseek(fd, offset, SEEK_SET);

    while (sent < total) {
        ssize_t rc = read(fd, m_ptr, total - sent);
        if (rc == 0) {
            break;
        }
        else if (rc < 0) {
            ret = (errno == EISDIR) ? Tsq::MountTaskFiletype : Tsq::MountTaskFailure;
            LOGDBG("Mount %p: read '%s': %m\n", this, req.name.c_str());
            goto out;
        }

        sent += rc;
        m_ptr += rc;
    }
out:
    pushReply(req.id, ret);
    if (needclose)
        close(fd);
}

void
FileMount::handleWrite(Req &req, Tsq::ProtocolUnmarshaler *unm, bool append)
{
    size_t offset = unm->parseNumber64();
    size_t total = unm->remainingLength(), sent = 0;
    const char *buf = unm->remainingBytes();

    if (m_ro || !req.name.empty()) {
        pushReply(req.id, Tsq::MountTaskFailure);
        return;
    }
    if (append) {
        // LOGDBG("Mount %p: append '%s': %lu\n", this, req.name.c_str(), total);
        lseek(m_fd, 0, SEEK_END);
    } else {
        // LOGDBG("Mount %p: write '%s': %lu at offset %lu\n", this, req.name.c_str(), total, offset);
        lseek(m_fd, offset, SEEK_SET);
    }

    while (sent < total) {
        ssize_t rc = write(m_fd, buf, total - sent);
        if (rc < 0) {
            if (errno == EINTR)
                continue;

            LOGDBG("Mount %p: write '%s': %m\n", this, req.name.c_str());
            pushReply(req.id, Tsq::MountTaskFailure);
            return;
        }
        sent += rc;
        buf += rc;
    }

    addNumber64(sent);
    pushReply(req.id, Tsq::MountTaskSuccess);
}

void
FileMount::handleChmod(Req &req, Tsq::ProtocolUnmarshaler *unm)
{
    if (!m_ro && req.name.empty())
        fchmod(m_fd, unm->parseNumber() & 0777);
}

void
FileMount::handleTrunc(Req &req, Tsq::ProtocolUnmarshaler *unm)
{
    if (!m_ro && req.name.empty())
        ftruncate(m_fd, unm->parseNumber64());
}

void
FileMount::handleTouch(Req &req)
{
    if (!m_ro && req.name.empty())
        futimes(m_fd, NULL);
}

void
FileMount::handleCreate(Req &req, Tsq::ProtocolUnmarshaler *unm)
{
    // LOGDBG("Mount %p: create '%s'\n", this, req.name.c_str());

    if (!m_ro && req.name.empty()) {
        ftruncate(m_fd, 0);
        fchmod(m_fd, unm->parseNumber() & 0777);
        handleStat(req);
    }
}

void
FileMount::handleOpen(Req &req)
{
    decltype(m_files)::iterator i;
    int fd;
    auto ret = Tsq::MountTaskSuccess;

    // LOGDBG("Mount %p: open '%s'\n", this, req.name.c_str());

    if (req.name.empty())
        goto out;

    i = m_files.find(req.name);
    if (i != m_files.end()) {
        ++i->second.second;
        goto out;
    }
    if (m_files.size() == MOUNT_MAX_HANDLES)
        goto out;
    if ((fd = openSubfd(req.name.c_str(), ret)) < 0)
        goto out;

    m_files.emplace(req.name, std::make_pair(fd, 1));
out:
    pushReply(req.id, ret);
}

void
FileMount::handleOpendir(Req &req)
{
    decltype(m_dirs)::iterator i;
    DIR *dir;
    int fd;
    auto ret = Tsq::MountTaskSuccess;

    // LOGDBG("Mount %p: opendir '%s'\n", this, req.name.c_str());

    if (req.name.empty())
        goto out;

    i = m_dirs.find(req.name);
    if (i != m_dirs.end()) {
        ++i->second.second;
        goto out;
    }

    // LOGDBG("Mount %p: opening handle to dir '%s'\n", this, req.name.c_str());

    if ((fd = openSubfd(req.name.c_str(), ret)) < 0) {
        goto out;
    }
    if (!(dir = fdopendir(fd))) {
        ret = (errno == ENOTDIR) ? Tsq::MountTaskFiletype : Tsq::MountTaskFailure;
        goto out;
    }

    m_dirs.emplace(req.name, std::make_pair(dir, 1));
out:
    pushReply(req.id, ret);
}

void
FileMount::handleReaddir(Req &req, Tsq::ProtocolUnmarshaler *unm)
{
    auto i = m_dirs.find(req.name);

    // LOGDBG("Mount %p: readdir '%s'\n", this, req.name.c_str());

    if (!m_isdir || i == m_dirs.end()) {
        pushReply(req.id, Tsq::MountTaskFailure);
        return;
    }

    DIR *dir = i->second.first;
    const struct dirent *ent;

    size_t total = unm->parseNumber64(), sent = 0;
    seekdir(dir, (long)unm->parseNumber64());
    unsigned overhead = unm->parseNumber();

    overhead += (4 - (overhead & 3)) & 3;
    if (overhead < 16) // our local overhead
        overhead = 16;

    setBufferSize(total);

    while ((ent = readdir(dir))) {
        unsigned mode;
        switch (ent->d_type) {
        case DT_DIR:
            mode = S_IFDIR;
            break;
        case DT_REG:
            mode = S_IFREG;
            break;
        default:
            continue;
        }

        unsigned len = strlen(ent->d_name);
        sent += overhead + len + 4 - (len & 3);
        if (sent > total)
            break;

        addNumber(mode);
        addNumber64(osTellDir(dir, ent));
        addPaddedString(ent->d_name, len);
        // LOGDBG("Mount %p:   dirent '%s'\n", this, ent->d_name);
    }

    pushReply(req.id, Tsq::MountTaskSuccess);
}

void
FileMount::handleClosedir(Req &req)
{
    // LOGDBG("Mount %p: closedir '%s'\n", this, req.name.c_str());

    if (req.name.empty())
        return;

    auto i = m_dirs.find(req.name);
    if (i != m_dirs.end() && --i->second.second == 0) {
        if (i->second.first != m_dir) {
            // LOGDBG("Mount %p: closing handle to dir '%s'\n", this, req.name.c_str());
            closedir(i->second.first);
        }

        m_dirs.erase(i);
    }
}

void
FileMount::handleClose(Req &req)
{
    // LOGDBG("Mount %p: close '%s'\n", this, req.name.c_str());

    if (req.name.empty())
        return;

    auto i = m_files.find(req.name);
    if (i != m_files.end() && --i->second.second == 0) {
        if (i->second.first != m_fd) {
            // LOGDBG("Mount %p: closing handle to file '%s'\n", this, req.name.c_str());
            close(i->second.first);
        }

        m_files.erase(i);
    }
}

bool
FileMount::handleIdle()
{
    if (m_reqs.empty()) {
        m_timeout = -1;
        return true;
    }

    auto &req = m_reqs.front();
    Tsq::ProtocolUnmarshaler unm(req.data.data(), req.data.size());
    // Reset reply pointer
    m_ptr = m_buf + COMMONSIZE;

    switch (req.op) {
    case Tsq::MountTaskOpInvalid:
        break;
    case Tsq::MountTaskOpLookup:
    case Tsq::MountTaskOpStat:
        handleStat(req);
        break;
    case Tsq::MountTaskOpRead:
    case Tsq::MountTaskOpDownload:
        handleRead(req, &unm);
        break;
    case Tsq::MountTaskOpWrite:
    case Tsq::MountTaskOpUpload:
        handleWrite(req, &unm, false);
        break;
    case Tsq::MountTaskOpAppend:
        handleWrite(req, &unm, true);
        break;
    case Tsq::MountTaskOpChmod:
        handleChmod(req, &unm);
        break;
    case Tsq::MountTaskOpTrunc:
        handleTrunc(req, &unm);
        break;
    case Tsq::MountTaskOpTouch:
        handleTouch(req);
        break;
    case Tsq::MountTaskOpCreate:
        handleCreate(req, &unm);
        break;
    case Tsq::MountTaskOpOpen:
        handleOpen(req);
        break;
    case Tsq::MountTaskOpClose:
        handleClose(req);
        break;
    case Tsq::MountTaskOpOpendir:
        handleOpendir(req);
        break;
    case Tsq::MountTaskOpReaddir:
        handleReaddir(req, &unm);
        break;
    case Tsq::MountTaskOpClosedir:
        handleClosedir(req);
        break;
    default:
        pushReply(req.id, Tsq::MountTaskFailure);
    }

    m_reqs.pop();
    return true;
}

bool
FileMount::openfd()
{
    bool retried = false;
retry:
    m_fd = open(m_targetName.c_str(), (m_ro ? O_RDONLY : O_RDWR)|O_CLOEXEC|O_NOCTTY);
    if (m_fd < 0) {
        if (errno == EISDIR && !retried) {
            m_ro = true;
            retried = true;
            goto retry;
        }

        LOGDBG("Mount %p: failed to open '%s': %m\n", this, m_targetName.c_str());
        reportError(Tsq::MountTaskErrorOpenFailed, strerror(errno));
        return false;
    }

    // Determine if this is a directory
    std::string mt;
    m_dir = fdopendir(m_fd);
    if (m_dir) {
        m_isdir = true;
        m_dirs.emplace(mt, std::make_pair(m_dir, 1));
    } else {
        m_files.emplace(mt, std::make_pair(m_fd, 1));
    }

    // Send starting information
    unsigned flags = (m_isdir << 1) | (m_ro << 0);
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addBytes(m_buf + 8, 48);
    m.addNumberPair(Tsq::TaskStarting, flags);
    g_listener->forwardToClient(m_clientId, m.result());
    LOGDBG("Mount %p: running\n", this);
    return true;
}

void
FileMount::threadMain()
{
    try {
        if (openfd())
            runDescriptorLoopWithoutFd();
    }
    catch (const std::exception &e) {
        LOGERR("Mount %p: caught exception: %s\n", this, e.what());
    }

    std::string mt;

    if (m_dir) {
        closedir(m_dir);
        m_fd = -1;

        m_dirs.erase(mt);
        for (const auto i: m_dirs)
            closedir(i.second.first);
    }
    else if (m_fd != -1) {
        close(m_fd);
        m_fd = -1;
    }

    m_files.erase(mt);
    for (const auto i: m_files)
        close(i.second.first);

    g_listener->sendWork(ListenerRemoveTask, this);
}
