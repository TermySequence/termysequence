// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QObject>

class Plugin;

class Feature: public QObject
{
    Q_OBJECT

public:
    enum Type { SemanticFeatureType, ActionFeatureType, TipFeatureType };

private:
    Type m_type;

protected:
    Plugin *m_plugin;
    QString m_name;

public:
    Feature(Type type, Plugin *parent, const QString &name);

    inline Type type() const { return m_type; }
    inline const QString& name() const { return m_name; }
    inline Plugin* plugin() { return m_plugin; }

    QString typeString() const;
};
