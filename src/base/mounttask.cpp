// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "app/icons.h"
#include "app/logging.h"
#include "mounttask.h"
#include "conn.h"
#include "listener.h"
#include "settings/launcher.h"
#include "os/dir.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/exception.h"

#include <QSocketNotifier>
#include <QUrl>
#include <QMimeData>
#include <unistd.h>

#include "moc_mounttask.cpp"

#define HAVE_FUSE (USE_FUSE3 || USE_FUSE2)
#define INITIALSIZE 8192
#define HEADERSIZE 56
#define MAXNAME 1024
#define MAXSIZE 88 + MAXNAME

#if HAVE_FUSE
#include "mountops.hpp"
#endif

#define TR_ERROR1 TL("error", "Remote mount feature disabled at compile time")
#define TR_TASKOBJ1 TL("task-object", "This computer")
#define TR_TASKOBJ2 TL("task-object", "File") + ':'
#define TR_TASKSTAT1 TL("task-status", "Idle")
#define TR_TASKSTAT2 TL("task-status", "In Use")
#define TR_TASKSTAT3 TL("task-status", "Timed Out")
#define TR_TASKTYPE1 TL("task-type", "Mount File Read-Write")
#define TR_TASKTYPE2 TL("task-type", "Mount File Read-Only")
#define TR_TASKTYPE3 TL("task-type", "Mount Folder Read-Only")

QHash<QString,MountTask*> MountTask::s_activeTasks;

MountTask::MountTask(ServerInstance *server, const QString &infile,
                     const QString &outname, bool ro) :
    TermTask(server, true),
    m_ro(ro),
    m_infile(infile),
    m_outfile(outname),
    m_outfilestr(outname.toStdString()),
    m_rootmap(m_dirs[m_mountpoint]),
    m_uid(getuid()),
    m_gid(getgid()),
    m_outbuf(new char[INITIALSIZE]),
    m_outsize(INITIALSIZE)
{
    m_typeStr = m_ro ? TR_TASKTYPE2 : TR_TASKTYPE1;
    m_typeIcon = ICON_TASKTYPE_MOUNT;
    m_toStr = TR_TASKOBJ1;
    m_sourceStr = TR_TASKOBJ2 + infile;

    uint32_t command = htole32(TSQ_TASK_INPUT);

    m_buf = new char[MAXSIZE];
    memcpy(m_buf, &command, 4);
    memcpy(m_buf + 8, server->id().buf, 16);
    memcpy(m_buf + 24, g_listener->id().buf, 16);
    memcpy(m_buf + 40, m_taskId.buf, 16);
}

void
MountTask::unmount()
{
#if HAVE_FUSE
    if (m_se) {
        m_notifier->setEnabled(false);
        s_activeTasks.remove(m_mountId);
#if USE_FUSE3
        fuse_session_unmount(m_se);
        fuse_session_destroy(m_se);
#else // USE_FUSE2
        fuse_session_remove_chan(m_ch);
        fuse_session_destroy(m_se);
        fuse_unmount(m_mountpoint.c_str(), m_ch);
        m_ch = nullptr;
#endif
        m_se = nullptr;
        free(m_fbuf.mem);
    }

    for (auto &i: m_inodes) {
        for (int fd: i.second->fds)
            close(fd);
        delete i.second;
    }

    if (!m_mountpoint.empty()) {
        rmdir(m_mountpoint.c_str());
        m_mountpoint.clear();
    }

    for (const auto &i: m_reqs)
        if (i.buf)
            mount_cleanup(i.op, i.buf);

    forDeleteAll(m_seensets);
    m_seensets.clear();
    m_inodes.clear();
    m_reqs.clear();
    fuse_opt_free_args(&m_args);

    if (m_timerId) {
        killTimer(m_timerId);
        m_timerId = 0;
    }
#endif
    m_unmounted = true;
    if (m_launcher) {
        m_launcher->putReference();
        m_launcher = nullptr;
    }
}

MountTask::~MountTask()
{
    unmount();
    delete [] m_buf;
    delete [] m_outbuf;
}

void
MountTask::setLauncher(LaunchSettings *launcher)
{
    (m_launcher = launcher)->takeReference();
    m_timeLimit = launcher->mountIdle();
    m_mountId = m_ro + ':' + m_server->idStr() + ':' + m_infile;
    disableStatusPopup();
}

void
MountTask::pushCancel(int code)
{
    Tsq::ProtocolMarshaler m(TSQ_CANCEL_TASK);
    m.addBytes(m_buf + 8, 48);
    m_server->conn()->push(m.resultPtr(), m.length());

    if (code)
        fail(strerror(code));
}

void
MountTask::prepMessage(Tsq::MountTaskOpcode op, const std::string &name)
{
    m_ptr = m_buf + HEADERSIZE;

    uint32_t val = htole32(m_nextReq);
    memcpy(m_ptr, &val, 4);
    val = htole32(op);
    memcpy(m_ptr + 4, &val, 4);
    m_ptr += 8;

    size_t skip = m_outfilestr.size() + 1;
    size_t len = name.size();
    len -= (skip < len ? skip : len);

    memcpy(m_ptr, name.data() + skip, len);
    m_ptr += len;
    unsigned padding = 4 - ((m_ptr - m_buf) & 3);
    memset(m_ptr, 0, padding);
    m_ptr += padding;
}

void
MountTask::prepRequest(Tsq::MountTaskOpcode op, const std::string &name,
                       fuse_req_t req, void *buf)
{
    Req state = { op, req, buf };

    if (m_nextReq < m_reqs.size()) {
        m_reqs[m_nextReq] = state;
    } else {
        m_nextReq = m_reqs.size();
        m_reqs.push_back(state);
    }

    prepMessage(op, name);
    m_nextReq = m_reqs.size();
}

void
MountTask::addNumber(uint32_t value)
{
    value = htole32(value);
    memcpy(m_ptr, &value, 4);
    m_ptr += 4;
}

void
MountTask::addNumber64(uint64_t value)
{
    value = htole64(value);
    memcpy(m_ptr, &value, 8);
    m_ptr += 8;
}

void
MountTask::pushRequest(const char *buf, size_t size)
{
    unsigned len = m_ptr - m_buf;
    uint32_t val = htole32(len + size - 8);
    memcpy(m_buf + 4, &val, 4);

    if (size) {
        m_server->conn()->send(m_buf, len);
        m_server->conn()->push(buf, size);
    } else {
        m_server->conn()->push(m_buf, len);
    }
}

void
MountTask::start(TermManager *manager)
{
    auto i = s_activeTasks.constFind(m_mountId), j = s_activeTasks.cend();

    if (!HAVE_FUSE) {
        failStart(manager, TR_ERROR1);
        unmount();
    }
    else if (m_launcher && i != j) {
        // The file is already mounted by another task
        MountTask *o = *i;
        if (o->m_timerId) {
            o->killTimer(o->m_timerId);
            o->m_timerId = o->startTimer(MOUNT_IDLE_TIMEOUT * 60000);
        }
        emit ready(o->m_mountfile);
        deleteLater();
    }
    else if (TermTask::doStart(manager)) {
        // Write task start
        auto type = m_ro ? TSQ_MOUNT_FILE_READONLY : TSQ_MOUNT_FILE_READWRITE;
        Tsq::ProtocolMarshaler m(type);
        m.addBytes(m_buf + 8, 48);
        m.addBytes(m_infile.toStdString());

        m_server->conn()->push(m.resultPtr(), m.length());
    }
    else {
        unmount();
    }
}

void
MountTask::handleResult(Tsq::ProtocolUnmarshaler *unm)
{
#if HAVE_FUSE
    unsigned idx = unm->parseNumber();
    if (idx >= m_reqs.size())
        return;
    Req item = m_reqs[idx];

    if (idx == m_reqs.size()) {
        m_reqs.pop_back();
        while (!m_reqs.empty() && m_reqs.back().op == Tsq::MountTaskOpInvalid)
            m_reqs.pop_back();
    } else {
        Req &ref = m_reqs[idx];
        ref.op = Tsq::MountTaskOpInvalid;
        ref.buf = nullptr;

        if (idx < m_nextReq)
            m_nextReq = idx;
    }

    bool done = true;

    switch (item.op) {
    case Tsq::MountTaskOpInvalid:
        return;
    case Tsq::MountTaskOpLookup:
        mount_lookup_result(unm, item.req, item.buf);
        break;
    case Tsq::MountTaskOpStat:
        mount_getattr_result(unm, item.req, item.buf);
        break;
    case Tsq::MountTaskOpOpen:
        mount_open_result(unm, item.req, item.buf);
        break;
    case Tsq::MountTaskOpRead:
        mount_read_result(unm, item.req, item.buf);
        break;
    case Tsq::MountTaskOpWrite:
    case Tsq::MountTaskOpAppend:
        mount_write_result(unm, item.req);
        break;
    case Tsq::MountTaskOpUpload:
        done = mount_replace_result(unm, item.req, item.buf);
        break;
    case Tsq::MountTaskOpDownload:
        done = mount_copy_result(unm, item.req, item.buf);
        break;
    case Tsq::MountTaskOpOpendir:
        mount_opendir_result(unm, item.req, item.buf);
        break;
    case Tsq::MountTaskOpReaddir:
        mount_readdir_result(unm, item.req, item.buf);
        break;
    case Tsq::MountTaskOpCreate:
        mount_create_result(unm, item.req, item.buf);
        break;
    default:
        break;
    }

    if (done && item.buf)
        mount_cleanup(item.op, item.buf);
#endif
}

void
MountTask::handleStart(unsigned flags)
{
#if HAVE_FUSE
    m_ro = flags & 1;
    m_isdir = flags & 2;
    m_typeStr = m_isdir ? TR_TASKTYPE3 : (m_ro ? TR_TASKTYPE2 : TR_TASKTYPE1);

    try {
        m_dirfd = osCreateMountPath(m_taskId.shortStr(), m_mountpoint);
    }
    catch (const std::exception &e) {
        m_mountpoint.clear();
        pushCancel(0);
        fail(e.what());
        return;
    }

    auto *irec = new Inode(m_nextInode = FUSE_ROOT_ID);
    irec->isdir = true;
    irec->fds.push_back(m_dirfd);
    m_inodes.emplace(irec->ino, irec);

    irec = new Inode(++m_nextInode);
    irec->isdir = m_isdir;
    irec->isremote = true;
    irec->name = m_outfilestr;
    m_inodes.emplace(irec->ino, irec);
    m_rootmap.emplace(m_outfilestr, irec);

    m_args.argc = ARRAY_SIZE(s_mountargv);
    if (!m_ro)
        m_args.argc -= 2;
    m_args.argv = (char **)s_mountargv;
#if USE_FUSE3
    struct fuse_cmdline_opts opts;
    fuse_parse_cmdline(&m_args, &opts);
#else // USE_FUSE2
    fuse_parse_cmdline(&m_args, NULL, NULL, NULL);
#endif

    struct fuse_lowlevel_ops mountops = { mountop_init };
    mountops.lookup = mountop_lookup;
    mountops.forget = mountop_forget;
    mountops.forget_multi = mountop_forget_multi;
    mountops.getattr = mountop_getattr;
    mountops.open = mountop_open;
    mountops.read = mountop_read;
    mountops.release = mountop_release;
    mountops.opendir = mountop_opendir;
    mountops.readdir = mountop_readdir;
    mountops.releasedir = mountop_releasedir;
    if (!m_ro) {
        mountops.setattr = mountop_setattr;
        mountops.mknod = mountop_mknod;
        mountops.mkdir = mountop_mkdir;
        mountops.unlink = mountop_unlink;
        mountops.symlink = mountop_symlink;
        mountops.rename = mountop_rename;
        mountops.link = mountop_link;
        mountops.create = mountop_create;
        mountops.write_buf = mountop_write;
    }

    int fd;
#if USE_FUSE3
    m_se = fuse_session_new(&m_args, &mountops, sizeof(mountops), this);
    if (!m_se) {
        pushCancel(EINVAL);
        goto mounterr;
    }
    if (fuse_session_mount(m_se, m_mountpoint.c_str()) != 0) {
        pushCancel(EINVAL);
        fuse_session_destroy(m_se);
        m_se = nullptr;
        goto mounterr;
    }
    fd = fuse_session_fd(m_se);
#else // USE_FUSE2
    m_ch = fuse_mount(m_mountpoint.c_str(), &m_args);
    if (!m_ch) {
        pushCancel(EINVAL);
        goto mounterr;
    }
    m_se = fuse_lowlevel_new(&m_args, &mountops, sizeof(mountops), this);
    if (!m_se) {
        fuse_unmount(m_mountpoint.c_str(), m_ch);
        pushCancel(EINVAL);
        goto mounterr;
    }
    fuse_session_add_chan(m_se, m_ch);
    fd = fuse_chan_fd(m_ch);

    m_fbuf.size = fuse_chan_bufsize(m_ch);
    m_fbuf.mem = malloc(m_fbuf.size);
#endif

    m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_notifier, SIGNAL(activated(int)), SLOT(handleFuse()));
    m_notifier->setEnabled(!m_throttles);

    m_mountfile = QString::fromStdString(m_mountpoint) + m_outfile;
    m_sinkStr = TR_TASKOBJ2 + m_mountfile;
    setStatus(TR_TASKSTAT1);
    emit taskChanged();

    if (m_launcher) {
        s_activeTasks[m_mountId] = this;
        if (m_timeLimit > 0)
            m_timerId = startTimer(MOUNT_IDLE_TIMEOUT * 60000);

        emit ready(m_mountfile);
        m_launcher->putReference();
        m_launcher = nullptr;
    } else {
        launch(m_mountfile);
    }
    return;
mounterr:
    rmdir(m_mountpoint.c_str());
    m_mountpoint.clear();
#endif
}

void
MountTask::handleOutput(Tsq::ProtocolUnmarshaler *unm)
{
    if (finished())
        return;

    switch (unm->parseNumber()) {
    case Tsq::TaskRunning:
        handleResult(unm);
        break;
    case Tsq::TaskStarting:
        handleStart(unm->parseNumber());
        break;
    case Tsq::TaskError:
        unmount();
        unm->parseNumber();
        fail(QString::fromStdString(unm->parseString()));
        break;
    default:
        break;
    }
}

void
MountTask::handleFuse()
{
#if HAVE_FUSE
    int rc = 0;

    if (!fuse_session_exited(m_se))
    {
#if USE_FUSE3
        rc = fuse_session_receive_buf(m_se, &m_fbuf);
#else // USE_FUSE2
        struct fuse_chan *ch = m_ch;
        struct fuse_buf buf = { m_fbuf.size, (fuse_buf_flags)0, m_fbuf.mem };
        rc = fuse_session_receive_buf(m_se, &buf, &ch);
#endif
        if (rc == -EINTR)
            return;
        if (rc <= 0)
            goto stop;

#if USE_FUSE3
        fuse_session_process_buf(m_se, &m_fbuf);
#else // USE_FUSE2
        fuse_session_process_buf(m_se, &buf, ch);
#endif
        return;
    }
stop:
    unmount();
    pushCancel(-rc);
#endif
}

void
MountTask::cancel()
{
    unmount();

    // Write task cancel
    if (!finished())
        pushCancel(0);

    TermTask::cancel();
}

void
MountTask::handleDisconnect()
{
    unmount();
}

void
MountTask::handleThrottle(bool throttled)
{
    if (throttled)
        ++m_throttles;
    else
        --m_throttles;

    if (m_notifier)
        m_notifier->setEnabled(!m_throttles);
}

MountTask::Seenset *
MountTask::createSeenset()
{
    auto seenset = new Seenset();
    m_seensets.emplace(seenset);
    return seenset;
}

void
MountTask::destroySeenset(Seenset *seenset)
{
    m_seensets.erase(seenset);
    delete seenset;
}

void
MountTask::setRenaming(bool renaming)
{
    if (renaming)
        ++m_throttles;
    else
        --m_throttles;

    m_notifier->setEnabled(!m_throttles);
}

void
MountTask::reportSent(size_t sent)
{
    m_sent += sent;
    queueTaskChange();
}

void
MountTask::reportReceived(size_t received)
{
    m_received += received;
    queueTaskChange();
}

void
MountTask::reportOpen()
{
    if (++m_users == 1) {
        if (m_timerId) {
            killTimer(m_timerId);
            m_timerId = 0;
        }
        setStatus(TR_TASKSTAT2);
        queueTaskChange();
    }
}

void
MountTask::reportClose()
{
    if (--m_users == 0) {
        if (m_timeLimit > 0) {
            m_timerId = startTimer(m_timeLimit * 60000);
        }
        setStatus(TR_TASKSTAT1);
        queueTaskChange();
    }
}

void
MountTask::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != m_timerId) {
        TermTask::timerEvent(event);
    } else {
        pushCancel(0);
        unmount();
        TermTask::cancel();
        setStatus(TR_TASKSTAT3);
        emit taskChanged();
    }
}

bool
MountTask::clonable() const
{
    return m_unmounted && m_target;
}

TermTask *
MountTask::clone() const
{
    return new MountTask(m_server, m_infile, m_outfile, m_ro);
}

QString
MountTask::launchfile() const
{
    return m_se ? m_mountfile : g_mtstr;
}

void
MountTask::getDragData(QMimeData *data) const
{
    if (m_se) {
        data->setText(m_mountfile);
        data->setUrls(QList<QUrl>{ QUrl::fromLocalFile(m_mountfile) });
    }
}
