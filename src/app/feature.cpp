// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "feature.h"
#include "plugin.h"

Feature::Feature(Type type, Plugin *parent, const QString &name) :
    QObject(parent),
    m_type(type),
    m_plugin(parent),
    m_name(name)
{
}

QString
Feature::typeString() const
{
    switch (m_type) {
    case SemanticFeatureType:
        return tr("semantic region parser");
    case ActionFeatureType:
        return tr("custom action");
    case TipFeatureType:
        return tr("tip of the day provider");
    default:
        return QString();
    }
}
