// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/logging.h"
#include "switchrule.h"

SwitchRuleset *g_switchrules;

enum SwitchParseError {
    ParseInvalidVerb,
    ParseInvalidRegex,
    ParseInvalidName,
    ParseInvalidFrom,
    ParseInvalidTo,
};

//
// Ruleset class
//
SwitchRuleset::SwitchRuleset(const QString &path) :
    m_path(path)
{
}

SwitchRuleset::SwitchRuleset(const SwitchRuleset &copyfrom) :
    m_rules(copyfrom.m_rules),
    m_varset(copyfrom.m_varset)
{
}

bool
SwitchRuleset::parseRule(const QStringList &parts, int lineno)
{
    SwitchRule rule;
    rule.fromVerb = SwitchRule::getFromVerb(parts[1]);
    rule.fromProfile = parts[2];
    rule.varVerb = SwitchRule::getVarVerb(parts[3]);
    rule.varName = parts[4];
    rule.varValue = parts[5];
    rule.toVerb = SwitchRule::getToVerb(parts[6]);
    rule.toProfile = parts[7];

    int reason;
    if (rule.isValid(&reason)) {
        m_rules.append(rule);
        return true;
    }

    qCWarning(lcSettings, "%s: Parse error on line %d:", pr(m_path), lineno);

    switch (reason) {
    case ParseInvalidVerb:
        qCWarning(lcSettings, "Unrecognized verb \"%s\"", pr(parts[3]));
        break;
    case ParseInvalidRegex:
        qCWarning(lcSettings, "Invalid regexp \"%s\"", pr(parts[5]));
        break;
    case ParseInvalidName:
        qCWarning(lcSettings, "Invalid variable \"%s\"", pr(parts[4]));
        break;
    case ParseInvalidFrom:
        qCWarning(lcSettings, "Unrecognized verb \"%s\"", pr(parts[1]));
        break;
    case ParseInvalidTo:
        qCWarning(lcSettings, "Unrecognized verb \"%s\"", pr(parts[6]));
        break;
    }
    return false;
}

bool
SwitchRuleset::parseFile(QFile &file)
{
    QRegularExpression ruleLine(L("\\A(\\w+)\\s+\"(.*)\"\\s+"
                                  "(!?\\w+)\\s+\"(\\S+)\"\\s+\"(.*)\"\\s+"
                                  "(\\w+)\\s+\"(.*)\"\\z"));

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
SwitchRuleset::postProcess()
{
    m_varset.clear();

    for (auto &i: m_rules) {
        m_varset.insert(i.varName);
        if (i.varVerb == VarVerbRegex || i.varVerb == VarVerbNotRegex)
            i.varRegex.setPattern(i.varValue);
    }
}

void
SwitchRuleset::load()
{
    QFile file(m_path.toUtf8().constData());

    m_rules.clear();

    if (file.exists() && !parseFile(file)) {
        qCWarning(lcSettings) << "Parse error while reading rules file" << m_path;
        m_rules.clear();
    }

    postProcess();
}

void
SwitchRuleset::save()
{
    QFile file(m_path.toUtf8().constData());

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(lcSettings, "Failed to open %s for writing: %s", pr(m_path), pr(file.errorString()));
        return;
    }

    for (auto &i: qAsConst(m_rules)) {
        QString line(L("%1 \"%2\" %3 \"%4\" \"%5\" %6 \"%7\"\n"));

        file.write(line.arg(SwitchRule::getFromVerbStr(i.fromVerb),
                            i.fromProfile,
                            SwitchRule::getVarVerbStr(i.varVerb),
                            i.varName,
                            i.varValue,
                            SwitchRule::getToVerbStr(i.toVerb),
                            i.toProfile).toUtf8());
    }
}

void
SwitchRuleset::setRules(const SwitchRuleset &rules)
{
    m_rules = rules.m_rules;
    postProcess();
}

int
SwitchRuleset::validateRules() const
{
    for (int i = 0; i < m_rules.size(); ++i)
        if (!m_rules[i].isValid())
            return i;

    return -1;
}

SwitchToVerb
SwitchRuleset::traverse(const AttributeMap &map, const QString &fromProfile,
                        QString &toProfile) const
{
    for (auto &i: qAsConst(m_rules))
        if (i.fromVerb == FromVerbAny || i.fromProfile == fromProfile)
            if (i.isMatch(map)) {
                toProfile = i.toProfile;
                return i.toVerb;
            }

    return ToVerbInvalid;
}


//
// Rule class
//
SwitchRule::SwitchRule() :
    fromVerb(FromVerbInvalid),
    toVerb(ToVerbInvalid),
    fromProfile(g_str_DEFAULT_PROFILE),
    toProfile(g_str_DEFAULT_PROFILE)
{
}

bool
SwitchRule::isValid(int *reasonret) const
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
    if (fromVerb == FromVerbInvalid) {
        reason = ParseInvalidFrom;
        goto err;
    }
    if (toVerb == ToVerbInvalid) {
        reason = ParseInvalidTo;
        goto err;
    }
    return true;
err:
    if (reasonret)
        *reasonret = reason;
    return false;
}

const char *
SwitchRule::getFromVerbName(SwitchFromVerb verb)
{
    switch (verb) {
    case FromVerbIs:
        return TN("SwitchRuleModel", "Is");
    case FromVerbAny:
        return TN("SwitchRuleModel", "Is anything");
    default:
        return TN("SwitchRuleModel", "<select>");
    }
}

const char *
SwitchRule::getToVerbName(SwitchToVerb verb)
{
    switch (verb) {
    case ToVerbSwitch:
        return TN("SwitchRuleModel", "Switch to profile");
    case ToVerbPush:
        return TN("SwitchRuleModel", "Push profile");
    case ToVerbPop:
        return TN("SwitchRuleModel", "Pop profile");
    default:
        return TN("SwitchRuleModel", "<select>");
    }
}

QStringList
SwitchRule::getFromVerbNames()
{
    QStringList result;
    TRAPPEND(getFromVerbName(FromVerbInvalid));
    TRAPPEND(getFromVerbName(FromVerbIs));
    TRAPPEND(getFromVerbName(FromVerbAny));
    return result;
}

QStringList
SwitchRule::getToVerbNames()
{
    QStringList result;
    TRAPPEND(getToVerbName(ToVerbInvalid));
    TRAPPEND(getToVerbName(ToVerbSwitch));
    TRAPPEND(getToVerbName(ToVerbPush));
    TRAPPEND(getToVerbName(ToVerbPop));
    return result;
}

SwitchFromVerb
SwitchRule::getFromVerb(const QString &verbStr)
{
    if (verbStr.compare(A("is"), Qt::CaseInsensitive) == 0)
        return FromVerbIs;
    if (verbStr.compare(A("any"), Qt::CaseInsensitive) == 0)
        return FromVerbAny;

    return FromVerbInvalid;
}

SwitchToVerb
SwitchRule::getToVerb(const QString &verbStr)
{
    if (verbStr.compare(A("switch"), Qt::CaseInsensitive) == 0)
        return ToVerbSwitch;
    if (verbStr.compare(A("push"), Qt::CaseInsensitive) == 0)
        return ToVerbPush;
    if (verbStr.compare(A("pop"), Qt::CaseInsensitive) == 0)
        return ToVerbPop;

    return ToVerbInvalid;
}

const char *
SwitchRule::getFromVerbStr(SwitchFromVerb verb)
{
    switch (verb) {
    case FromVerbIs:
        return "is";
    case FromVerbAny:
        return "any";
    default:
        return "";
    }
}

const char *
SwitchRule::getToVerbStr(SwitchToVerb verb)
{
    switch (verb) {
    case ToVerbSwitch:
        return "switch";
    case ToVerbPush:
        return "push";
    case ToVerbPop:
        return "pop";
    default:
        return "";
    }
}
