// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "attributebase.h"

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <dirent.h>

class TermInstance;
struct FileInfo;
struct inotify_event;
struct git_repository;
namespace Tsq { class ProtocolMarshaler; }

class TermFilemon final: public AttributeBase
{
private:
    TermInstance *m_parent;

    DIR *m_dir = nullptr;
    int m_dirFd = -1;
    int m_notifyFd = -1;
    int m_notifyWd;
    int m_gitWd;

    unsigned m_nfiles;
    unsigned m_limit;

    std::string m_path;
    std::unordered_map<uint32_t,std::string> m_users, m_groups;
    std::unordered_set<std::string*> m_incomingDirectories;

    // Git monitoring
    struct git_repository *m_git = nullptr;
#if USE_LIBGIT2
    std::string m_gitdir, m_gitrel;

    class GitRepo {
        TermFilemon *m_parent;
    public:
        GitRepo(TermFilemon *parent);
        ~GitRepo();

        struct git_repository *repo;
        std::string dir;
    };

    struct GitCache {
        std::shared_ptr<GitRepo> rptr;
        std::string rel;
        int64_t time;
    };

    std::unordered_map<std::string,GitCache> m_gitcache;
    std::unordered_map<std::string,std::weak_ptr<GitRepo>> m_gitrepos;
    std::unordered_set<int> m_gitWatches;
#endif

private:
    void describeGit(Tsq::ProtocolMarshaler *m);
    void openGit(Tsq::ProtocolMarshaler *m);
    void findGit(int64_t now);
    void setGitWatches();
    void checkGit(const std::string &name, Tsq::ProtocolMarshaler *m);
    bool handleGitEvent(const struct inotify_event *e);

private:
    void closefd();
    std::string buildMsg(const std::string &name, const FileInfo *info, bool git);

    void reportDirectoryUpdate(const std::string &msg);
    void reportDirectoryOverlimit();
    void reportFileRemoved(const std::string &name, const std::string &msg);
    bool reportFileUpdate(const std::string &name, std::string &&msg);

    bool handleNotifyEvent(const struct inotify_event *e);
    void handleNotifyFd();
    void handleDirFd();

    void threadMain();
    bool handleFd();
    bool handleWork(const WorkItem &item);
    void handleDirectory(std::string *dir);
    void handleRelimit(unsigned limit);

public:
    TermFilemon(unsigned limit, TermInstance *parent);
    ~TermFilemon();

    // state locked
    const auto& files() const { return m_attributes; }

    void monitor(const std::string &directory);
    void setLimit(const std::string &value);
};

enum FilemonWork {
    FilemonClose,
    FilemonDirectory,
    FilemonRelimit,
};
