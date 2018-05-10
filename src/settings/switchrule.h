// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "varrule.h"

#include <QSet>
#include <QFile>

enum SwitchFromVerb {
    FromVerbInvalid, FromVerbIs, FromVerbAny
};
enum SwitchToVerb {
    ToVerbInvalid, ToVerbSwitch, ToVerbPush, ToVerbPop
};

struct SwitchRule: VarRule
{
    SwitchFromVerb fromVerb;
    SwitchToVerb toVerb;

    QString fromProfile;
    QString toProfile;

    static const char* getFromVerbName(SwitchFromVerb verb);
    static const char* getToVerbName(SwitchToVerb verb);
    static QStringList getFromVerbNames();
    static QStringList getToVerbNames();

    static SwitchFromVerb getFromVerb(const QString &verbStr);
    static SwitchToVerb getToVerb(const QString &verbStr);
    static const char* getFromVerbStr(SwitchFromVerb verb);
    static const char* getToVerbStr(SwitchToVerb verb);

public:
    SwitchRule();

    bool isValid(int *reasonret = nullptr) const;
};

class SwitchRuleset
{
private:
    QList<SwitchRule> m_rules;
    QSet<QString> m_varset;

    QString m_path;

    bool parseRule(const QStringList &parts, int lineno);
    bool parseFile(QFile &file);

    void postProcess();

public:
    SwitchRuleset() = default;
    SwitchRuleset(const QString &path);
    SwitchRuleset(const SwitchRuleset &copyfrom);

    inline const QList<SwitchRule>& rules() const { return m_rules; }
    inline int size() const { return m_rules.size(); }
    inline const SwitchRule& at(int i) const { return m_rules.at(i); }
    inline SwitchRule& rule(int i) { return m_rules[i]; }

    inline void swap(int i, int j) { m_rules.swap(i, j); }
    inline void move(int from, int to) { m_rules.move(from, to); }
    inline void removeAt(int i) { m_rules.removeAt(i); }
    inline void insert(int i, const SwitchRule &rule = SwitchRule())
    { m_rules.insert(i, rule); }

    inline bool hasVariable(const QString &varName) const
    { return m_varset.contains(varName); }

    void load();
    void save();
    void setRules(const SwitchRuleset &rules);
    int validateRules() const;

    SwitchToVerb traverse(const AttributeMap &map, const QString &fromProfile,
                          QString &toProfile) const;
};

extern SwitchRuleset *g_switchrules;
