// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "app/attrbase.h"

#include <QRegularExpression>

enum VarVerb {
    VarVerbInvalid, VarVerbIs, VarVerbNotIs, VarVerbSet, VarVerbNotSet,
    VarVerbStarts, VarVerbNotStarts, VarVerbEnds, VarVerbNotEnds,
    VarVerbContains, VarVerbNotContains, VarVerbRegex, VarVerbNotRegex
};

struct VarRule
{
    VarVerb varVerb;
    QString varName, varValue;
    QRegularExpression varRegex;

    static const char* getVarVerbName(VarVerb verb);
    static QStringList getVarVerbNames();

    static VarVerb getVarVerb(const QString &verbStr);
    static const char* getVarVerbStr(VarVerb verb);

public:
    VarRule();
    inline VarRule(VarVerb v, const QString &vn, const char *vv) :
        varVerb(v), varName(vn), varValue(vv) {}

    bool isMatch(const AttributeMap &map) const;
    bool isMatch(const QString &value) const;
};

#define TRAPPEND(x) result.append(QCoreApplication::translate("SwitchRuleModel", x))
