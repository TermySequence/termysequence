// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "varrule.h"

VarRule::VarRule() :
    varVerb(VarVerbInvalid),
    varName(A("<name>")),
    varValue(A("<value>"))
{
}

bool
VarRule::isMatch(const AttributeMap &map) const
{
    if (!map.contains(varName))
        return varVerb == VarVerbNotSet;

    bool isPos = varVerb % 2;

    switch (varVerb + isPos) {
    case VarVerbNotIs:
        return (map[varName] == varValue) == isPos;
    case VarVerbNotSet:
        return isPos;
    case VarVerbNotStarts:
        return map[varName].startsWith(varValue) == isPos;
    case VarVerbNotEnds:
        return map[varName].endsWith(varValue) == isPos;
    case VarVerbNotContains:
        return map[varName].contains(varValue) == isPos;
    case VarVerbNotRegex:
        return varRegex.match(map[varName]).hasMatch() == isPos;
    default:
        return false;
    }
}

bool
VarRule::isMatch(const QString &value) const
{
    bool isPos = varVerb % 2;

    switch (varVerb + isPos) {
    case VarVerbNotIs:
        return (value == varValue) == isPos;
    case VarVerbNotSet:
        return isPos;
    case VarVerbNotStarts:
        return value.startsWith(varValue) == isPos;
    case VarVerbNotEnds:
        return value.endsWith(varValue) == isPos;
    case VarVerbNotContains:
        return value.contains(varValue) == isPos;
    case VarVerbNotRegex:
        return varRegex.match(value).hasMatch() == isPos;
    default:
        return false;
    }
}

const char *
VarRule::getVarVerbName(VarVerb verb)
{
    switch (verb) {
    case VarVerbIs:
        return TN("VarRuleModel", "Is");
    case VarVerbNotIs:
        return TN("VarRuleModel", "Is not");
    case VarVerbSet:
        return TN("VarRuleModel", "Is set");
    case VarVerbNotSet:
        return TN("VarRuleModel", "Is not set");
    case VarVerbStarts:
        return TN("VarRuleModel", "Starts with");
    case VarVerbNotStarts:
        return TN("VarRuleModel", "Does not start with");
    case VarVerbEnds:
        return TN("VarRuleModel", "Ends with");
    case VarVerbNotEnds:
        return TN("VarRuleModel", "Does not end with");
    case VarVerbContains:
        return TN("VarRuleModel", "Contains");
    case VarVerbNotContains:
        return TN("VarRuleModel", "Does not contain");
    case VarVerbRegex:
        return TN("VarRuleModel", "Matches regex");
    case VarVerbNotRegex:
        return TN("VarRuleModel", "Does not match regex");
    default:
        return TN("VarRuleModel", "<select>");
    }
}

QStringList
VarRule::getVarVerbNames()
{
    QStringList result;
    TRAPPEND(getVarVerbName(VarVerbInvalid));
    TRAPPEND(getVarVerbName(VarVerbIs));
    TRAPPEND(getVarVerbName(VarVerbNotIs));
    TRAPPEND(getVarVerbName(VarVerbSet));
    TRAPPEND(getVarVerbName(VarVerbNotSet));
    TRAPPEND(getVarVerbName(VarVerbStarts));
    TRAPPEND(getVarVerbName(VarVerbNotStarts));
    TRAPPEND(getVarVerbName(VarVerbEnds));
    TRAPPEND(getVarVerbName(VarVerbNotEnds));
    TRAPPEND(getVarVerbName(VarVerbContains));
    TRAPPEND(getVarVerbName(VarVerbNotContains));
    TRAPPEND(getVarVerbName(VarVerbRegex));
    TRAPPEND(getVarVerbName(VarVerbNotRegex));
    return result;
}

VarVerb
VarRule::getVarVerb(const QString &verbStr)
{
    bool isNeg = verbStr.startsWith('!');
    QStringRef verbRef(&verbStr, isNeg, verbStr.size() - isNeg);

    if (verbRef.compare(A("is"), Qt::CaseInsensitive) == 0)
        return (VarVerb)(VarVerbIs + isNeg);
    if (verbRef.compare(A("set"), Qt::CaseInsensitive) == 0)
        return (VarVerb)(VarVerbSet + isNeg);
    if (verbRef.compare(A("startswith"), Qt::CaseInsensitive) == 0)
        return (VarVerb)(VarVerbStarts + isNeg);
    if (verbRef.compare(A("endswith"), Qt::CaseInsensitive) == 0)
        return (VarVerb)(VarVerbEnds + isNeg);
    if (verbRef.compare(A("contains"), Qt::CaseInsensitive) == 0)
        return (VarVerb)(VarVerbContains + isNeg);
    if (verbRef.compare(A("regex"), Qt::CaseInsensitive) == 0)
        return (VarVerb)(VarVerbRegex + isNeg);

    return VarVerbInvalid;
}

const char *
VarRule::getVarVerbStr(VarVerb verb)
{
    bool isPos = verb % 2;

    switch (verb + isPos) {
    default:
        return &"!is"[isPos];
    case VarVerbNotSet:
        return &"!set"[isPos];
    case VarVerbNotStarts:
        return &"!startswith"[isPos];
    case VarVerbNotEnds:
        return &"!endswith"[isPos];
    case VarVerbNotContains:
        return &"!contains"[isPos];
    case VarVerbNotRegex:
        return &"!regex"[isPos];
    }
}
