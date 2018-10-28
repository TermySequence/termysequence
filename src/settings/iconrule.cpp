// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/logging.h"
#include "iconrule.h"
#include "base/thumbicon.h"

#include "app/deficon.hpp"

IconRuleset *g_iconrules;

enum IconParseError {
    ParseInvalidVerb,
    ParseInvalidRegex,
    ParseInvalidName,
    ParseInvalidIcon,
};

//
// Ruleset class
//
IconRuleset::IconRuleset()
{
    resetToDefaults();
}

IconRuleset::IconRuleset(const QString &path) :
    m_path(path)
{
}

IconRuleset::IconRuleset(const IconRuleset &copyfrom) :
    m_rules(copyfrom.m_rules),
    m_varset(copyfrom.m_varset)
{
}

bool
IconRuleset::parseRule(const QStringList &parts, int lineno)
{
    IconRule rule;
    rule.varVerb = IconRule::getVarVerb(parts[1]);
    rule.varName = parts[2];
    rule.varValue = parts[3];
    rule.toIcon = parts[4];

    int reason;
    if (rule.isValid(&reason)) {
        m_rules.append(rule);
        return true;
    }

    qCWarning(lcSettings, "%s: Parse error on line %d:", pr(m_path), lineno);

    switch (reason) {
    case ParseInvalidVerb:
        qCWarning(lcSettings, "Unrecognized verb \"%s\"", pr(parts[1]));
        break;
    case ParseInvalidRegex:
        qCWarning(lcSettings, "Invalid regexp \"%s\"", pr(parts[3]));
        break;
    case ParseInvalidName:
        qCWarning(lcSettings, "Invalid variable \"%s\"", pr(parts[2]));
        break;
    case ParseInvalidIcon:
        qCWarning(lcSettings, "Invalid icon \"%s\"", pr(parts[4]));
        break;
    }
    return false;
}

bool
IconRuleset::parseFile(QFile &file)
{
    QRegularExpression ruleLine(L("\\A(!?\\w+)\\s+\"(\\S+)\"\\s+\"(.*)\"\\s+"
                                  "\"(.*)\"\\z"));

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(lcSettings, "Failed to open %s: %s", pr(m_path), pr(file.errorString()));
        return false;
    }

    QTextStream in(&file);
    int lineno = 0;

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        ++lineno;

        if (line.isEmpty())
            continue;
        if (line.startsWith('#'))
            continue;

        auto match = ruleLine.match(line);

        if (match.hasMatch()) {
            if (!parseRule(match.capturedTexts(), lineno))
                return false;
        } else {
            qCWarning(lcSettings, "%s: Parse error on line %d: Unrecognized rule", pr(m_path), lineno);
            return false;
        }
    }

    return true;
}

void
IconRuleset::postProcess()
{
    m_varset.clear();

    for (auto &i: m_rules) {
        m_varset.insert(i.varName);
        if (i.varVerb == VarVerbRegex || i.varVerb == VarVerbNotRegex)
            i.varRegex.setPattern(i.varValue);
    }
}

void
IconRuleset::resetToDefaults()
{
    m_rules.clear();

    unsigned n = ARRAY_SIZE(s_defaultIconRules);
    for (unsigned i = 0; i < n; ++i)
        m_rules.append(s_defaultIconRules[i]);
}

void
IconRuleset::load()
{
    QFile file(m_path.toUtf8().constData());

    if (file.exists()) {
        if (!parseFile(file)) {
            qCWarning(lcSettings) << "Parse error while reading rules file" << m_path;
            resetToDefaults();
        }
    } else {
        resetToDefaults();
    }

    postProcess();
}

void
IconRuleset::save()
{
    QFile file(m_path.toUtf8().constData());

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(lcSettings, "Failed to open %s for writing: %s", pr(m_path), pr(file.errorString()));
        return;
    }

    for (auto &i: qAsConst(m_rules)) {
        QString line(L("%1 \"%2\" \"%3\" \"%4\"\n"));

        file.write(line.arg(IconRule::getVarVerbStr(i.varVerb),
                            i.varName,
                            i.varValue,
                            i.toIcon).toUtf8());
    }
}

void
IconRuleset::setRules(const IconRuleset &rules)
{
    m_rules = rules.m_rules;
    postProcess();
}

int
IconRuleset::validateRules() const
{
    for (int i = 0; i < m_rules.size(); ++i)
        if (!m_rules[i].isValid())
            return i;

    return -1;
}

int
IconRuleset::findCommand(QString &inout) const
{
    inout = inout.section(' ', 0, 0, QString::SectionSkipEmpty);
    if (inout == A("cd"))
        return -1;

    for (auto &i: qAsConst(m_rules))
        if (i.varName == g_attr_PROC_COMM && i.isMatch(inout)) {
            inout = i.toIcon;
            return i.toIcon != TI_NONE_NAME;
        }

    return 0;
}

//
// Rule class
//
IconRule::IconRule() :
    toIcon(A("<icon>"))
{
}

bool
IconRule::isValid(int *reasonret) const
{
    int reason;

    switch (varVerb) {
    case VarVerbInvalid:
        reason = ParseInvalidVerb;
        goto err;
    case VarVerbRegex:
    case VarVerbNotRegex:
        if (!QRegularExpression(varValue).isValid()) {
            reason = ParseInvalidRegex;
            goto err;
        }
        break;
    default:
        break;
    }

    if (varName.isEmpty() || varName == A("<name>")) {
        reason = ParseInvalidName;
        goto err;
    }
    if (toIcon == A("<icon>")) {
        reason = ParseInvalidIcon;
        goto err;
    }
    return true;
err:
    if (reasonret)
        *reasonret = reason;
    return false;
}
