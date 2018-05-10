// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/flags.h"

#include <map>
#include <deque>
#include <QString>
#include <QHash>

#define RULEFLAG_SHIFT     0
#define RULEFLAG_CONTROL   1
#define RULEFLAG_ALT       2
#define RULEFLAG_META      3
#define RULEFLAG_ANYMOD    4
#define RULEFLAG_KEYPAD    5
#define RULEFLAG_COMMAND   6
#define RULEFLAG_SELECT    7
#define RULEFLAG_APPSCREEN 8
#define RULEFLAG_APPCUKEYS 9
#define RULEFLAG_APPKEYPAD 10
#define RULEFLAG_NEWLINE   11
#define RULEFLAG_ANSI      12
#define RULEFLAG_N_FLAGS   13

struct KeymapRule;
struct OrderedKeymapRule;

//
// Set-based rule container
//
class KeymapRuleset
{
private:
    std::multimap<int,KeymapRule*> m_set;

public:
    KeymapRuleset() = default;
    KeymapRuleset(const KeymapRuleset &other);
    ~KeymapRuleset();
    KeymapRuleset& operator=(const KeymapRuleset &other);

    inline size_t size() const { return m_set.size(); }

    inline auto begin() const { return m_set.cbegin(); }
    inline auto end() const { return m_set.cend(); }
    inline auto equal_range(int key) const { return m_set.equal_range(key); }

    void insert(KeymapRule *rule); // takeown
    void clear();
};

//
// List-based rule container
//
class KeymapRulelist
{
private:
    std::deque<OrderedKeymapRule*> m_list;

public:
    KeymapRulelist() = default;
    KeymapRulelist(const KeymapRulelist &other);
    ~KeymapRulelist();

    inline auto& at(size_t pos) { return m_list.at(pos); }
    inline const auto& at(size_t pos) const { return m_list.at(pos); }
    inline size_t size() const { return m_list.size(); }

    inline auto begin() const { return m_list.begin(); }
    inline auto end() const { return m_list.end(); }
    inline auto rbegin() const { return m_list.rbegin(); }
    inline auto rend() const { return m_list.rend(); }

    void erase(size_t pos);
    OrderedKeymapRule* take(size_t pos);
    void insert(size_t pos, OrderedKeymapRule *rule); // takeown
    void append(OrderedKeymapRule *rule); // takeown
    void clear();
};

//
// Plain rule
//
struct TermShortcut
{
    Tsq::TermFlags conditions;
    Tsq::TermFlags mask;
    QString expression;
    QString additional;
};

struct KeymapRule
{
    Tsq::TermFlags conditions;
    Tsq::TermFlags mask;
    int key;
    bool startsCombo;
    bool continuesCombo;
    bool special;
    QString keyName;
    QByteArray outcome;
    KeymapRuleset comboRuleset;

    QString outcomeStr;
    QString modifierStr;
    QString expressionStr;
    QString additionalStr;

    void parse(const QStringList &parts, bool special);
    void populateStrings();
    void constructShortcut(TermShortcut &result) const;

    void write(QByteArray &result) const;

    static QByteArray parseBytes(QString spec);
    static const char* getFlagName(int flag);
    static const char* getFlagDescription(int flag);
    static Tsq::TermFlags getFlagValue(int flag);
};

//
// List-based rule
//
struct OrderedKeymapRule: public KeymapRule
{
    KeymapRulelist comboRulelist;
    int index;
    int parentIndex;
    int priority;
    int maxPriority;

    OrderedKeymapRule(const KeymapRule &rule, int index, int parentIndex = -1);
    OrderedKeymapRule();
};

inline void
KeymapRule::constructShortcut(TermShortcut &result) const
{
    result.mask = mask & ~Tsqt::ModMask;
    result.conditions = conditions;
    result.expression = expressionStr;
    result.additional = additionalStr;
}

inline bool
operator==(const TermShortcut &a, const TermShortcut &b)
{
    return a.conditions == b.conditions && a.mask == b.mask &&
        a.expression == b.expression;
}

inline uint
qHash(const TermShortcut &a, uint seed)
{
    return qHash(a.conditions) ^ qHash(a.mask) ^ qHash(a.expression, seed);
}
