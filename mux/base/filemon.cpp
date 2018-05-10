// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "filemon.h"
#include "term.h"
#include "exception.h"
#include "app/args.h"
#include "os/fd.h"
#include "os/time.h"
#include "os/user.h"
#include "os/logging.h"
#include "os/git.h"
#include "os/filemon.h"
#include "lib/wire.h"
#include "lib/protocol.h"
#include "lib/attr.h"
#include "lib/attrstr.h"
#include "config.h"

#include <cstdio>
#include <unistd.h>

#define PATHSIZE 1024

TermFilemon::TermFilemon(unsigned limit, TermInstance *parent) :
    AttributeBase("filemon"),
    m_parent(parent),
    m_limit(limit)
{
    m_id = m_parent->id();
}

TermFilemon::~TermFilemon()
{
    for (auto i: m_incomingDirectories)
        delete i;
}

/*
 * Other threads
 */
void
TermFilemon::setLimit(const std::string &value)
{
    const char *startptr = value.c_str();
    char *endptr;
    unsigned num = strtoul(startptr, &endptr, 10);

    if (*startptr && !*endptr)
        sendWork(FilemonRelimit, num);
}

void
TermFilemon::monitor(const std::string &directory)
{
    std::string *copy = new std::string(directory);

    FlexLock lock(this);

    m_incomingDirectories.insert(copy);
    sendWorkAndUnlock(lock, FilemonDirectory, copy);
}

/*
 * This thread
 */
#if USE_LIBGIT2
bool
TermFilemon::handleGitEvent(const struct inotify_event *e)
{
    if ((m_gitWd == e->wd && e->len &&
         (!strcmp(e->name, "index") || !strcmp(e->name, "HEAD") ||
          !strcmp(e->name, "FETCH_HEAD") || !strcmp(e->name, "config"))) ||
        m_gitWatches.count(e->wd))
    {
        // start over
        // LOGDBG("Filemon %p: starting over (git, watch %d)\n", this, e->wd);
        monitor(m_path);
        return false;
    }

    return true;
}

void
TermFilemon::setGitWatches()
{
    // Watch several files under .git
    std::string tmp = m_gitdir + ".git";
    m_gitWd = inotify_add_watch(m_notifyFd, tmp.c_str(),
        IN_CLOSE_WRITE|IN_CREATE|IN_DELETE|IN_MOVE|
        IN_EXCL_UNLINK|IN_ONLYDIR);

    tmp = m_gitdir + ".git/info/exclude";
    m_gitWatches.insert(inotify_add_watch(m_notifyFd, tmp.c_str(),
        IN_CLOSE_WRITE|IN_MOVE_SELF|IN_ONESHOT));

    // Watch folders for .gitignore changes
    std::string rel = m_gitrel;

    while (1) {
        while (!rel.empty() && rel.back() == '/') {
            rel.pop_back();
        }
        if (rel.empty()) {
            break;
        }

        size_t idx = rel.find_last_of('/');
        if (idx == std::string::npos)
            idx = 0;
        else
            ++idx;

        rel.erase(idx);
        tmp = m_gitdir + rel;

        std::string gi = tmp + "/.gitignore";
        if (osFileExists(gi.c_str())) {
            // LOGDBG("Filemon %p: add gitignore watch on '%s'\n", this, gi.c_str());
            m_gitWatches.insert(inotify_add_watch(m_notifyFd, gi.c_str(),
                              IN_CLOSE_WRITE|IN_MOVE_SELF|IN_ONESHOT));
        }
    }
}

inline void
TermFilemon::describeGit(Tsq::ProtocolMarshaler *m)
{
    git_reference *head, *upstream;
    const char *branch;

    if (git_repository_head(&head, m_git) == 0)
    {
        if (git_repository_head_detached(m_git) != 0) {
            git_object *headobj;
            git_describe_result *desc;
            git_describe_options opts = GIT_DESCRIBE_OPTIONS_INIT;
            git_describe_format_options fopts = GIT_DESCRIBE_FORMAT_OPTIONS_INIT;
            git_buf buf = { 0 };

            opts.describe_strategy = GIT_DESCRIBE_TAGS;
            opts.show_commit_oid_as_fallback = 1;

            if (git_reference_peel(&headobj, head, GIT_OBJ_ANY) == 0) {
                if (git_describe_commit(&desc, headobj, &opts) == 0) {
                    if (git_describe_format(&buf, desc, &fopts) == 0) {
                        m->addStringPair(TSQ_ATTR_DIR_GIT, buf.ptr);
                        git_buf_free(&buf);
                    }
                    git_describe_result_free(desc);
                }
                git_object_free(headobj);
            }

            m->addStringPair(TSQ_ATTR_DIR_GIT_DETACHED, "");
        }
        else if (git_branch_name(&branch, head) == 0) {
            m->addStringPair(TSQ_ATTR_DIR_GIT, branch);
        }

        if (git_branch_upstream(&upstream, head) == 0) {
            git_oid hoid, uoid;
            size_t ahead, behind;

            if (git_reference_name_to_id(&hoid, m_git, git_reference_name(head)) == 0 &&
                git_reference_name_to_id(&uoid, m_git, git_reference_name(upstream)) == 0 &&
                git_graph_ahead_behind(&ahead, &behind, m_git, &hoid, &uoid) == 0)
            {
                m->addStringPair(Tsq::attr_DIR_GIT_AHEAD, std::to_string(ahead));
                m->addStringPair(Tsq::attr_DIR_GIT_BEHIND, std::to_string(behind));
            }

            m->addStringPair(TSQ_ATTR_DIR_GIT_TRACK, git_reference_shorthand(upstream));
            git_reference_free(upstream);
        }

        git_reference_free(head);
    }
}

TermFilemon::GitRepo::GitRepo(TermFilemon *parent)
{
    m_parent = parent;
}

TermFilemon::GitRepo::~GitRepo()
{
    m_parent->m_gitrepos.erase(dir);
    git_repository_free(repo);
}

void
TermFilemon::findGit(int64_t now)
{
    if (m_gitcache.size() == GIT_CACHE_MAX) {
        // Make a hole
        for (auto i = m_gitcache.begin(); i != m_gitcache.end(); ) {
            if (now - i->second.time > GIT_CACHE_TIMEOUT)
                i = m_gitcache.erase(i);
            else
                ++i;
        }
        if (m_gitcache.size() == GIT_CACHE_MAX) {
            m_gitcache.erase(m_gitcache.begin());
        }
    }

    // Find the .git folder
    std::shared_ptr<GitRepo> rptr;
    const char *gitdir;
    decltype(m_gitrepos)::iterator i;
    size_t len;

    m_gitdir = m_path;
    if (!osFindDirUpwards(m_gitdir, "/.git")) {
        m_git = nullptr;
        goto out;
    }

    len = m_gitdir.size();
    while (len < m_path.size() && m_path[len] == '/')
        ++len;
    m_gitrel = m_path.substr(len);

    // Find or open the repo
    if ((i = m_gitrepos.find(m_gitdir)) != m_gitrepos.end()) {
        rptr = i->second.lock();
        m_git = rptr->repo;
        m_gitdir = rptr->dir;
    }
    else if (git_repository_open_ext(&m_git, m_gitdir.c_str(), GIT_REPOSITORY_OPEN_NO_SEARCH, 0)) {
        m_git = nullptr;
    }
    else if (!(gitdir = git_repository_workdir(m_git)) || m_gitdir != gitdir) {
        git_repository_free(m_git);
        m_git = nullptr;
    }
    else {
        rptr = std::make_shared<GitRepo>(this);
        rptr->repo = m_git;
        rptr->dir = m_gitdir;
        m_gitrepos.emplace(m_gitdir, rptr);
    }

out:
    if (!m_git)
        m_gitrel.clear();

    struct GitCache centry = { rptr, m_gitrel, now };
    m_gitcache.emplace(m_path, std::move(centry));
}

void
TermFilemon::openGit(Tsq::ProtocolMarshaler *m)
{
    int64_t now = osMonotime();

    auto i = m_gitcache.find(m_path);
    if (i != m_gitcache.end()) {
        if (i->second.rptr) {
            m_git = i->second.rptr->repo;
            m_gitdir = i->second.rptr->dir;
            m_gitrel = i->second.rel;
        } else {
            m_git = nullptr;
        }
        i->second.time = now;
    } else {
        findGit(now);
    }

    if (m_git) {
        // LOGDBG("Filemon %p: gitdir='%s', gitrel='%s'\n", this,
        //        m_gitdir.c_str(), m_gitrel.c_str());

        if (m_notifyFd != -1)
            setGitWatches();

        describeGit(m);
    }
}

inline void
TermFilemon::checkGit(const std::string &name, Tsq::ProtocolMarshaler *m)
{
    unsigned flags;
    std::string tmp = m_gitrel + name;

    if (git_status_file(&flags, m_git, tmp.c_str()) == 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%x", flags);
        m->addStringPair(TSQ_ATTR_FILE_GIT, buf);
    }
}
#endif // USE_LIBGIT2

void
TermFilemon::closefd()
{
    if (m_dir) {
        closedir(m_dir); // closes dirFd
        m_dir = nullptr;
        m_dirFd = -1;
    }
    if (m_notifyFd != -1) {
        close(m_notifyFd);
        m_notifyFd = -1;
    }
#if USE_LIBGIT2
    if (m_git) {
        m_git = nullptr;
        m_gitWatches.clear();
    }
#endif // USE_LIBGIT2
    setfd(-1);
}

inline void
TermFilemon::reportDirectoryUpdate(const std::string &msg)
{
    {
        StateLock slock(this, true);
        m_attributes.clear();
        m_attributes.emplace(g_mtstr, msg);
    }

    m_parent->reportDirectoryUpdate(msg);
}

void
TermFilemon::reportDirectoryOverlimit()
{
    Tsq::ProtocolMarshaler m(TSQ_DIRECTORY_UPDATE, 16, m_id.buf);
    m.addNumber64(osWalltime());
    m.addString(m_path);
    m.addStringPair(Tsq::attr_FILE_OVERLIMIT, std::to_string(m_limit));
    reportDirectoryUpdate(m.result());
}

inline void
TermFilemon::reportFileRemoved(const std::string &name, const std::string &msg)
{
    {
        StateLock slock(this, true);
        m_attributes.erase(name);
    }

    --m_nfiles;
    m_parent->reportFileUpdate(name, msg);
}

inline bool
TermFilemon::reportFileUpdate(const std::string &name, std::string &&msg)
{
    auto i = m_attributes.find(name), j = m_attributes.end();
    if (i != j) {
        if (i->second == msg) {
            // Don't report duplicate events
            return true;
        }
    } else if (++m_nfiles > m_limit) {
        reportDirectoryOverlimit();
        return false;
    }
    {
        StateLock slock(this, true);
        if (i == j)
            i = m_attributes.emplace(name, msg).first;
        else
            i->second = msg;
    }

    m_parent->reportFileUpdate(name, i->second);
    return true;
}

std::string
TermFilemon::buildMsg(const std::string &name, const FileInfo *info, bool git)
{
    Tsq::ProtocolMarshaler m(TSQ_FILE_UPDATE, 16, m_id.buf);
    m.addNumber64(info->mtime);
    m.addNumber64(info->size);
    m.addNumber(info->mode);
    m.addNumberPair(info->uid, info->gid);
    m.addString(name);

    for (const auto &i: info->fattr)
        m.addStringPair(i.first, i.second);

    if (m_users.count(info->uid) == 0) {
        std::string tmp = osUserName(info->uid);
        m.addStringPair(TSQ_ATTR_FILE_USER, tmp.c_str());
        m_users[info->uid] = std::move(tmp);
    }
    if (m_groups.count(info->gid) == 0) {
        std::string tmp = osGroupName(info->gid);
        m.addStringPair(TSQ_ATTR_FILE_GROUP, tmp.c_str());
        m_groups[info->gid] = std::move(tmp);
    }

    if (USE_LIBGIT2 && m_git && git)
        checkGit(name, &m);

    return m.result();
}

bool
TermFilemon::handleNotifyEvent(const struct inotify_event *e)
{
    if (e->mask & (IN_DELETE_SELF|IN_MOVE_SELF|IN_IGNORED)) {
        Tsq::ProtocolMarshaler m(TSQ_DIRECTORY_UPDATE, 16, m_id.buf);
        m.addNumber64(osWalltime());
        m.addString(m_path);
        m.addStringPair(TSQ_ATTR_FILE_ERROR, strerror(ENOENT));
        reportDirectoryUpdate(m.result());
        return false;
    }

    if (e->len == 0)
        return true;

    std::string name(e->name);

    if (e->mask & (IN_DELETE|IN_MOVED_FROM)) {
        if (m_attributes.count(name)) {
            Tsq::ProtocolMarshaler m(TSQ_FILE_REMOVED, 16, m_id.buf);
            m.addNumber64(osWalltime());
            m.addBytes(name);
            reportFileRemoved(name, m.result());
        }
    }
    else {
        FileInfo info(m_dirFd);
        int rc = osStatFile(e->name, &info);
        if (rc != -1) {
            std::string msg = buildMsg(name, &info, rc == 1);
            return reportFileUpdate(name, std::move(msg));
        }
    }

    return true;
}

void
TermFilemon::handleNotifyFd()
{
    char buf[4 * PATHSIZE];
    ssize_t rc = read(m_notifyFd, buf, sizeof(buf));

    if (rc <= 0) {
        // Note: just give up on inotify on any error
        LOGERR("Filemon %p: inotify error: %s\n", this, strerror(errno));
        closefd();
    }
    else {
        for (const char *ptr = buf; ptr < buf + rc; ) {
            auto *e = (const struct inotify_event *)ptr;

            if (e->wd == -1) {
                // overflow, start over
                // LOGDBG("Filemon %p: starting over (overflow)\n", this);
                monitor(m_path);
                break;
            }
            else if (e->wd == m_notifyWd) {
                if (!handleNotifyEvent(e)) {
                    closefd();
                    break;
                }
            }

            // Git needs to see all notifications
            if (USE_LIBGIT2 && m_git && !handleGitEvent(e)) {
                break;
            }

            ptr += sizeof(*e) + e->len;
        }
    }
}

void
TermFilemon::handleDirFd()
{
    AttributeMap map;
    FileInfo info(m_dirFd);
    std::string msg;
    int rc;

    for (int i = 0; i < FILEMON_BATCH_SIZE; ++i)
    {
        switch (rc = osReadDir(m_dir, &info)) {
        default:
            if (++m_nfiles > m_limit) {
                closefd();
                reportDirectoryOverlimit();
                return;
            }
            msg = buildMsg(info.name, &info, rc == 2);
            map.emplace(std::move(info.name), std::move(msg));
            info.fattr.clear();
            // fallthru
        case 0:
            continue;
        case -1:
            // LOGDBG("Filemon %p: reading finished\n", this);
            setfd(m_notifyFd);
        }
        break;
    }

    if (!map.empty()) {
        {
            StateLock slock(this, true);

            for (const auto &i: map)
                m_attributes[i.first] = i.second;
        }

        m_parent->reportFileUpdates(map);
    }
}

bool
TermFilemon::handleFd()
{
    if (m_fd == m_dirFd)
        handleDirFd();
    else
        handleNotifyFd();

    return true;
}

void
TermFilemon::handleDirectory(std::string *dir)
{
    {
        Lock lock(this);
        m_incomingDirectories.erase(dir);
    }

    closefd();
    m_nfiles = 0;
    m_users.clear();
    m_groups.clear();
    m_path = std::move(*dir);
    delete dir;

    if (m_path.empty())
        return;
    if (m_path.back() != '/')
        m_path.push_back('/');

    const char *path = m_path.c_str();
    Tsq::ProtocolMarshaler m(TSQ_DIRECTORY_UPDATE, 16, m_id.buf);
    m.addNumber64(osWalltime());
    m.addString(m_path);

    if (m_path.size() >= PATHSIZE) {
        m.addStringPair(TSQ_ATTR_FILE_ERROR, strerror(ENAMETOOLONG));
        goto out;
    }
    if ((m_dirFd = osOpenDir(path, &m_dir)) < 0) {
        m.addStringPair(TSQ_ATTR_FILE_ERROR, strerror(errno));
        goto out;
    }
    if ((m_notifyFd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC)) != -1) {
        m_notifyWd = inotify_add_watch(m_notifyFd, path,
            IN_ATTRIB|IN_MODIFY|IN_CREATE|IN_DELETE|IN_MOVE|
            IN_DELETE_SELF|IN_MOVE_SELF|IN_EXCL_UNLINK|IN_ONLYDIR);
    }

    if (USE_LIBGIT2 && g_args->git())
        openGit(&m);

    // LOGDBG("Filemon %p: reading %s\n", this, path);
    setfd(m_dirFd);
out:
    reportDirectoryUpdate(m.result());
}

void
TermFilemon::handleRelimit(unsigned limit)
{
    m_limit = limit;
    // start over
    // LOGDBG("Filemon %p: starting over (relimit)\n", this);
    handleDirectory(new std::string(m_path));
}

bool TermFilemon::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case FilemonClose:
        return false;
    case FilemonDirectory:
        handleDirectory((std::string *)item.value);
        break;
    case FilemonRelimit:
        handleRelimit((unsigned)item.value);
        break;
    default:
        break;
    }

    return true;
}

void
TermFilemon::threadMain()
{
    try {
        runDescriptorLoop();
    }
    catch (const std::exception &e) {
        LOGERR("Filemon %p: caught exception: %s\n", this, e.what());
    }

    closefd();

#if USE_LIBGIT2
    m_gitcache.clear();
#endif // USE_LIBGIT2
}
