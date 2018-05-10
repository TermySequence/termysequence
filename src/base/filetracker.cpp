// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "filetracker.h"
#include "term.h"
#include "settings/profile.h"

#include <QDateTime>

#define TR_TEXT1 TL("window-text", "No information received from server")

//
// Model
//
FileTracker::FileTracker(TermInstance *parent) :
    QObject(parent),
    Dircolors(parent->palette()),
    m_term(parent),
    m_state(&m_cur)
{
    m_cur.dir.iserror = true;
    m_cur.dir.error = TR_TEXT1;

    memset(m_next.counts, 0, sizeof(m_next.counts));

    connect(m_term, SIGNAL(fileSettingsChanged()), SLOT(handleSettingsChanged()));
    connect(m_term, SIGNAL(paletteChanged()), SLOT(handlePaletteChanged()));

    const auto *profile = m_term->profile();
    m_showDotfiles = profile->showDotfiles();
    m_classify = profile->fileClassify();
    m_gittify = profile->fileGittify();
    m_colorize = profile->fileColorize();
    m_animate = profile->fileEffect();
}

void
FileTracker::updateFileDircolors(TermFile &file) const
{
    if (file.islink) {
        if (m_categories[file.linkcategory].flags & CategorySet) {
            file.lattr = m_categories[file.linkcategory];
            file.lattr.flags &= Tsq::All;
        } else {
            int idx = file.link.lastIndexOf('.');
            if (idx != -1) {
                auto i = m_extensions.constFind(file.link.mid(idx + 1));
                if (i != m_extensions.cend())
                    file.lattr = *i;
            }
        }
        if (m_categories[file.category].flags == UseLinkTarget) {
            file.fattr = file.lattr;
            return;
        }
    }

    if (m_categories[file.category].flags & CategorySet) {
        file.fattr = m_categories[file.category];
        file.fattr.flags &= Tsq::All;
    } else {
        auto i = m_extensions.constFind(file.ext);
        if (i != m_extensions.cend()) {
            file.fattr = *i;
        }
    }
}

void
FileTracker::updateFileDisplay(TermFile &file)
{
    file.classify = m_classify;
    file.gittify = m_gittify && file.isgit;
    file.fattr.flags = 0;
    file.lattr.flags = 0;

    if (m_colorize) {
        updateFileDircolors(file);

        if ((file.fattr.flags|file.lattr.flags) & Tsq::Blink)
            m_blinkSeen = true;
    }

    if (file.gittify) {
        Tsq::CellFlags gflags = 0;

        if (file.gitflags & Tsq::GitStatusIgnored) {
            gflags = m_categories[GitIgnored].flags;
            goto out;
        }
        if (file.gitflags & Tsq::GitStatusUnmerged) {
            gflags = m_categories[GitUnmerged].flags;
            goto out;
        }

        if (file.gitflags & Tsqt::GitStatusAnyIndex) {
            gflags = m_categories[GitIndex].flags;
        }
        if (file.gitflags & Tsqt::GitStatusAnyWorking) {
            gflags |= m_categories[GitWorking].flags;
        }
    out:
        if (gflags & Tsq::Blink)
            m_blinkSeen = true;
    }
}

int
FileTracker::getGitDisplay(const TermFile *file, CellAttributes gattr[2], QString gstr[2]) const
{
    if (file->gitflags & Tsq::GitStatusIgnored) {
        gattr[0] = m_categories[GitIgnored];
        gattr[0].flags &= Tsq::All;
        gstr[0] = g_str_GIT_I;
        return 1;
    }
    if (file->gitflags & Tsq::GitStatusUnmerged) {
        gattr[0] = m_categories[GitUnmerged];
        gattr[0].flags &= Tsq::All;
        gstr[0] = g_str_GIT_U;
        return 1;
    }

    int ret = 0;

    if (file->gitflags & Tsqt::GitStatusAnyIndex) {
        gattr[0] = m_categories[GitIndex];
        gattr[0].flags &= Tsq::All;

        if (file->gitflags & Tsq::GitStatusINew)
            gstr[0] = g_str_GIT_A;
        else if (file->gitflags & Tsq::GitStatusIDeleted)
            gstr[0] = g_str_GIT_D;
        else if (file->gitflags & Tsq::GitStatusIRenamed)
            gstr[0] = g_str_GIT_R;
        else if (file->gitflags & Tsq::GitStatusIModified)
            gstr[0] = g_str_GIT_S;
        else
            gstr[0] = g_str_GIT_T;

        ++ret;
    }
    if (file->gitflags & Tsqt::GitStatusAnyWorking) {
        gattr[ret] = m_categories[GitWorking];
        gattr[ret].flags &= Tsq::All;

        if (file->gitflags & Tsq::GitStatusWNew)
            gstr[ret] = C('?');
        else if (file->gitflags & Tsq::GitStatusWDeleted)
            gstr[ret] = g_str_GIT_D;
        else if (file->gitflags & Tsq::GitStatusWRenamed)
            gstr[ret] = g_str_GIT_R;
        else if (file->gitflags & Tsq::GitStatusWModified)
            gstr[ret] = g_str_GIT_M;
        else
            gstr[ret] = g_str_GIT_T;

        ++ret;
    }

    return ret;
}

void
FileTracker::handleSettingsChanged()
{
    const auto *profile = m_term->profile();
    m_showDotfiles = profile->showDotfiles();
    m_classify = profile->fileClassify();
    m_gittify = profile->fileGittify();
    m_colorize = profile->fileColorize();
    m_animate = profile->fileEffect();

    // Update all the existing files
    emit directoryChanging();

    for (auto &i: m_cur.files)
        updateFileDisplay(i);
    for (auto &i: m_next.files)
        updateFileDisplay(i);

    emit directoryChanged(false);
}

void
FileTracker::handlePaletteChanged()
{
    if (Dircolors::operator!=(m_term->palette())) {
        Dircolors::operator=(m_term->palette());

        // Update all the existing files
        emit directoryChanging();

        for (auto &i: m_cur.files)
            updateFileDisplay(i);
        for (auto &i: m_next.files)
            updateFileDisplay(i);

        emit directoryChanged(false);
    }
}

inline void
FileTracker::Dirstate::assign(TermDirectory &dir_)
{
    files.clear();
    map.clear();
    users.clear();
    groups.clear();
    dir = std::move(dir_);
    memset(counts, 0, sizeof(counts));
}

void
FileTracker::setDirectory(TermDirectory &dir)
{
    if (m_timerId != 0)
        killTimer(m_timerId);

    if (m_state->dir.sameAs(dir)) {
        m_state->dir = std::move(dir);
        m_seenset.clear();
        m_accumulating = true;

        if (!m_staging)
            emit directoryChanged(true);

        m_added = false;
        m_timerId = startTimer(DIRDEFER_TIME);
    }
    else {
        m_accumulating = false;

        if (dir.iserror) {
            m_timerId = 0;
            m_staging = false;
            emit directoryChanging();
            m_cur.assign(dir);
            m_state = &m_cur;
            emit directoryChanged(false);
        } else {
            m_next.assign(dir);
            m_state = &m_next;
            m_staging = true;

            m_added = false;
            m_timerId = startTimer(DIRDEFER_TIME);
        }
    }
}

inline void
FileTracker::doEmitUpdated(int row, unsigned changes)
{
    if (!m_staging)
        emit fileUpdated(row, changes);
}

inline void
FileTracker::doEmitAdding(int row)
{
    if (!m_staging)
        emit fileAdding(row);
}

inline void
FileTracker::doEmitAdded(int row)
{
    if (!m_staging)
        emit fileAdded(row);
    else
        m_added = true;
}

inline void
FileTracker::doEmitRemoving(int row)
{
    if (!m_staging)
        emit fileRemoving(row);
}

inline void
FileTracker::doEmitRemoved()
{
    if (!m_staging)
        emit fileRemoved();
}

void
FileTracker::updateFile(TermFile &file)
{
    bool userupdate = false;

    if (file.user.isEmpty()) {
        file.user = m_state->users.value(file.uid);
    }
    else if (!m_state->users.contains(file.uid)) {
        m_state->users.insert(file.uid, file.user);
        userupdate = true;
    }
    if (file.group.isEmpty()) {
        file.group = m_state->groups.value(file.gid);
    }
    else if (!m_state->groups.contains(file.gid)) {
        m_state->groups.insert(file.gid, file.group);
        userupdate = true;
    }

    if (userupdate)
        for (int i = 0; i < m_state->files.size(); ++i) {
            auto &other = m_state->files[i];
            bool changed = false;

            if (other.uid == file.uid && other.user.isEmpty()) {
                other.user = file.user;
                changed = true;
            }
            if (other.gid == file.gid && other.group.isEmpty()) {
                other.group = file.group;
                changed = true;
            }

            if (changed) {
                doEmitUpdated(i, TermFile::FcUser|TermFile::FcGroup);
            }
        }

    if (file.name != A(".") && file.name != A(".."))
    {
        if (m_accumulating) {
            m_seenset.insert(file.name);
            m_added = true;
        }

        auto i = m_state->map.constFind(file.name);
        if (i != m_state->map.cend()) {
            auto &ref = m_state->files[*i];
            unsigned changes = ref.compareTo(file);
            if (changes) {
                updateFileDisplay(file);
                --m_state->counts[ref.countindex];
                ++m_state->counts[file.countindex];
                ref = std::move(file);
                doEmitUpdated(*i, changes);
            }
        }
        else {
            updateFileDisplay(file);
            int row = m_state->files.size();
            m_state->map[file.name] = row;
            ++m_state->counts[file.countindex];
            doEmitAdding(row);
            m_state->files.append(std::move(file));
            doEmitAdded(row);
        }
    }
}

FileTracker::MapIterator
FileTracker::doRemove(MapIterator i)
{
    int row = *i;
    doEmitRemoving(row);
    --m_state->counts[m_state->files.at(row).countindex];
    i = m_state->map.erase(i);
    m_state->files.removeAt(row);
    doEmitRemoved();

    // Update rows for following files
    while (row < m_state->files.size()) {
        --m_state->map[m_state->files.at(row).name];
        ++row;
    }

    return i;
}

void
FileTracker::removeFile(const QString &name)
{
    auto i = m_state->map.constFind(name);
    if (i != m_state->map.cend())
        doRemove(i);
}

void
FileTracker::timerEvent(QTimerEvent *)
{
    if (m_added) {
        m_added = false;
        return;
    }

    killTimer(m_timerId);
    m_timerId = 0;

    if (m_staging) {
        m_staging = false;
        emit directoryChanging();
        m_cur = std::move(m_next);
        m_state = &m_cur;
        emit directoryChanged(false);

        m_next.files.clear(); // for settings updates
    }
    if (m_accumulating) {
        m_accumulating = false;

        for (auto i = m_cur.map.cbegin(); i != m_cur.map.cend(); ) {
            if (m_seenset.contains(i.key()))
                ++i;
            else
                i = doRemove(i);
        }
    }
}
