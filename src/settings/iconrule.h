// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "varrule.h"

#include <QSet>
#include <QFile>

struct IconRule: VarRule
{
    QString toIcon;

public:
    IconRule();
    inline IconRule(VarVerb v, const QString &vn, const char *vv, const char *i) :
        VarRule(v, vn, vv), toIcon(i) {}

    bool isValid(int *reason = nullptr) const;
};

class IconRuleset
{
private:
    QList<IconRule> m_rules;
    QSet<QString> m_varset;

    QString m_path;

    bool parseRule(const QStringList &parts, int lineno);
    bool parseFile(QFile &file);

    void postProcess();

public:
    IconRuleset();
    IconRuleset(const QString &path);
    IconRuleset(const IconRuleset &copyfrom);

    inline const auto& rules() const { return m_rules; }
    inline const auto& varset() const { return m_varset; }
    inline int size() const { return m_rules.size(); }
    inline const IconRule& at(int i) const { return m_rules.at(i); }
    inline IconRule& rule(int i) { return m_rules[i]; }

    inline void swap(int i, int j) { m_rules.swap(i, j); }
    inline void move(int from, int to) { m_rules.move(from, to); }
    inline void removeAt(int i) { m_rules.removeAt(i); }
    inline void insert(int i, const IconRule &rule = IconRule())
    { m_rules.insert(i, rule); }

    inline bool hasVariable(const QString &varName) const
    { return m_varset.contains(varName); }

    void load();
    void save();
    void setRules(const IconRuleset &rules);
    void resetToDefaults();
    int validateRules() const;

    QString traverse(const AttributeMap &map) const;
    int findCommand(QString &cmdInOut) const;
};

extern IconRuleset *g_iconrules;

inline QString
IconRuleset::traverse(const AttributeMap &map) const
{
    for (auto &i: qAsConst(m_rules))
        if (i.isMatch(map))
            return i.toIcon;

    return A("default");
}
