// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "file.h"
#include "settings/dircolors.h"

#include <QObject>
#include <QVector>
#include <QHash>
#include <QSet>

class TermInstance;

//
// Model
//
class FileTracker final: public QObject, public Dircolors
{
    Q_OBJECT

private:
    TermInstance *m_term;

    struct Dirstate {
        QVector<TermFile> files;
        QHash<QString,int> map;

        QHash<uint32_t,QString> users;
        QHash<uint32_t,QString> groups;

        TermDirectory dir;
        unsigned counts[TermFile::NFileCounts];

        void assign(TermDirectory &dir);
    };

    Dirstate *m_state;
    Dirstate m_cur, m_next;

    QSet<QString> m_seenset;

    bool m_showDotfiles;
    bool m_classify;
    bool m_gittify;
    bool m_colorize;
    char m_animate;
    bool m_blinkSeen = false;

    bool m_staging = false;
    bool m_accumulating = false;
    bool m_added;
    int m_timerId = 0;

    void updateFileDircolors(TermFile &file) const;
    void updateFileDisplay(TermFile &file);

    void doEmitUpdated(int row, unsigned changes);
    void doEmitAdding(int row);
    void doEmitAdded(int row);
    void doEmitRemoving(int row);
    void doEmitRemoved();

    typedef QHash<QString,int>::const_iterator MapIterator;
    MapIterator doRemove(MapIterator i);

private slots:
    void handleSettingsChanged();
    void handlePaletteChanged();

protected:
    void timerEvent(QTimerEvent *event);

signals:
    void directoryChanging();
    void directoryChanged(bool attronly);

    void fileUpdated(int row, unsigned changes);
    void fileAdding(int row);
    void fileAdded(int row);
    void fileRemoving(int row);
    void fileRemoved();

public:
    FileTracker(TermInstance *parent);

    inline const TermDirectory& dir() const { return m_cur.dir; }
    inline const TermDirectory& realdir() const { return m_state->dir; }
    inline const unsigned* counts() const { return m_cur.counts; }

    inline bool showDotfiles() const { return m_showDotfiles; }
    inline int animate() const { return m_animate; }
    inline bool blinkSeen() const { return m_blinkSeen; }

    void setDirectory(TermDirectory &dir);
    void updateFile(TermFile &file);
    void removeFile(const QString &name);

    int getGitDisplay(const TermFile *file, CellAttributes gattr[2], QString gstr[2]) const;

    inline int size() const { return m_cur.files.size(); }
    inline const TermFile& at(int i) const { return m_cur.files.at(i); }
};
