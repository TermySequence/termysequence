// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/enums.h"
#include "app/fuseutil.h"
#include "task.h"

#include <QHash>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>

QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE
namespace Tsq { class ProtocolMarshaler; }
class LaunchSettings;

typedef uint64_t inode_t;

class MountTask final: public TermTask
{
    Q_OBJECT

public:
    struct Inode {
        bool isinvalid;
        bool isdir;
        bool isremote;
        unsigned refcount;

        inode_t ino;
        std::string name;
        std::vector<int> fds;

        Inode(inode_t ino);
    };

    typedef std::map<std::string,Inode*> Dirmap;
    typedef std::unordered_set<std::string> Seenset;

private:
    struct Req {
        Tsq::MountTaskOpcode op;
        fuse_req_t req;
        void *buf;
    };

    std::vector<Req> m_reqs;
    unsigned m_nextReq = 0;
    char *m_buf, *m_ptr;

    struct fuse_buf m_fbuf{};
    struct fuse_session *m_se = nullptr;
    struct fuse_args m_args{};
    QSocketNotifier *m_notifier = nullptr;
    int m_dirfd;

    bool m_ro;
    bool m_isdir;
    bool m_unmounted = false;

    unsigned m_users = 0;
    unsigned m_throttles = 0;

    QString m_infile, m_outfile;
    QString m_mountfile;
    std::string m_mountpoint;

    std::string m_outfilestr;
    std::unordered_map<inode_t,Inode*> m_inodes;
    std::unordered_map<std::string,Dirmap> m_dirs;
    Dirmap &m_rootmap;
    inode_t m_nextInode;

    int m_uid, m_gid;
    char *m_outbuf;
    size_t m_outsize;

    std::unordered_set<Seenset*> m_seensets;

    LaunchSettings *m_launcher = nullptr;
    int m_timerId = 0, m_timeIdle = 0, m_timeLimit = 0;

    QString m_mountId;
    static QHash<QString,MountTask*> s_activeTasks;

    void unmount();
    void pushCancel(int code);

    void handleResult(Tsq::ProtocolUnmarshaler *unm);
    void handleStart(unsigned flags);

private slots:
    void handleThrottle(bool throttled);
    void handleFuse();

protected:
    void timerEvent(QTimerEvent *event);

public:
    MountTask(ServerInstance *server, const QString &infile,
              const QString &outname, bool readonly);
    ~MountTask();

    void setLauncher(LaunchSettings *launcher);

    void start(TermManager *manager);
    void handleOutput(Tsq::ProtocolUnmarshaler *unm);
    void handleDisconnect();
    void cancel();

    bool clonable() const;
    TermTask* clone() const;
    QString launchfile() const;
    void getDragData(QMimeData *data) const;

public:
    // Used by mountops
    inline Dirmap& dirmap(const std::string &name) { return m_dirs[name]; }
    inline Dirmap& rootmap() { return m_rootmap; }
    inline const std::string& file() const { return m_outfilestr; }
    inline int dirfd() { return m_dirfd; }
    inline int uid() const { return m_uid; }
    inline int gid() const { return m_gid; }

    Inode* lookupInode(inode_t ino) const;
    Inode* createInode(const std::string &name, int fd = -1);
    Inode* addInode(Dirmap &dirmap, const std::string &dir, const std::string &name);
    void unrefInode(inode_t ino, unsigned count);
    void unrefInode(Inode *irec);

    Inode* renameFile(const std::string &oldname, const std::string &newname);
    void unlinkFile(const std::string &name);

    char* outbuf(size_t size);
    char* outbuf(size_t size, bool preserved);

    void prepMessage(Tsq::MountTaskOpcode op, const std::string &name);
    void prepRequest(Tsq::MountTaskOpcode op, const std::string &name,
                        fuse_req_t req, void *p = nullptr);
    void addNumber(uint32_t value);
    void addNumber64(uint64_t value);
    void pushRequest(const char *payload = nullptr, size_t size = 0);

    Seenset* createSeenset();
    void destroySeenset(Seenset *seenset);

    void setRenaming(bool renaming);
    void reportSent(size_t sent);
    void reportReceived(size_t received);
    void reportOpen();
    void reportClose();
};
