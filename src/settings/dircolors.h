// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/cell.h"

#include <QHash>
#include <QVector>

//
// Main class
//
class Dircolors
{
public:
    enum Category {
        None,
        File,
        Dir,
        Link,
        Fifo,
        Sock,
        Door,
        Blk,
        Chr,
        Orphan,
        Missing,
        SetUID,
        SetGID,
        Setcap,
        StickyOtherW,
        OtherW,
        Sticky,
        Exec,
        GitIndex,
        GitWorking,
        GitIgnored,
        GitUnmerged,
        NCategories
    };
    enum SpecialFlag : uint32_t {
        // Indicates that a category is set
        CategorySet = 0x80000000,
        // Indicates that the "target" ln category option is set
        UseLinkTarget = 0xc0000000,
    };

protected:
    CellAttributes32 m_categories[NCategories];
    QHash<QString,CellAttributes32> m_extensions;

    QString m_spec;
    mutable QString m_env;

    std::pair<bool,bool> parse(const QString &spec);

private:
    void setDefaults1();
    void setDefaults2();

public:
    Dircolors();
    Dircolors(const QString &spec, bool *parseOk = nullptr);

    inline const auto categories() const { return m_categories; }
    inline const QString& dStr() const { return m_spec; }
    const QString& envStr() const;

    bool operator==(const Dircolors &other) const;
    bool operator!=(const Dircolors &other) const;

public:
    struct Entry {
        QString id;
        QString spec;
        int start;
        int end;
        CellAttributes32 attr;
        int type;
        unsigned char category;
    };
};

//
// Display class
//
class DircolorsDisplay final: public Dircolors
{
private:
    bool m_inherits;

    void putEnt(Entry &ent, QVector<Entry> &ilist, QVector<Entry> &list);

public:
    inline bool inherits() const { return m_inherits; }

    void parseForDisplay(const QString &spec, QVector<Entry> &result);
};

inline bool
Dircolors::operator==(const Dircolors &other) const
{
    return (m_spec == other.m_spec) ||
        (memcmp(m_categories, other.m_categories, sizeof(m_categories)) == 0 &&
         m_extensions == other.m_extensions);
}

inline bool
Dircolors::operator!=(const Dircolors &other) const
{
    return (m_spec != other.m_spec) &&
        (memcmp(m_categories, other.m_categories, sizeof(m_categories)) ||
         m_extensions != other.m_extensions);
}
