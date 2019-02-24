// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "v8util.h"

#include <QObject>
#include <QHash>
#include <QVector>
#include <QFileInfo>

class Feature;
class SemanticFeature;
class ActionFeature;

class Plugin final: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString pluginName READ name WRITE setName)
    Q_PROPERTY(QString pluginDescription READ description WRITE setDescription)
    Q_PROPERTY(QString pluginVersion READ version WRITE setVersion)

private:
    PersistentContext m_context;

    QFileInfo m_path;
    QString m_name, m_description, m_version;

    QHash<QString,PersistentModule> m_modules;
    QHash<PersistentModule,QString> m_modlocs;

    QVector<Feature*> m_features;
    QVector<SemanticFeature*> m_semantics;

    bool m_loaded;

signals:
    void featureUnloaded(Feature *feature);

public:
    Plugin(const QFileInfo &path, const QString &name);

    inline auto context() { return LocalContext::New(i, m_context); }
    inline bool loaded() const { return m_loaded; }

    inline const QString& name() const { return m_name; }
    inline const QString& description() const { return m_description; }
    inline const QString& version() const { return m_version; }

    inline const auto& features() const { return m_features; }
    inline const auto& semantics() const { return m_semantics; }

    void addFeature(Feature *feature);
    void unloadFeature(const QString &featureName);

    void setName(const QString &name);
    void setDescription(const QString &description);
    void setVersion(const QString &version);

    bool loadModule(const QString &path, LocalModule &result, bool isroot);
    LocalModule getModule(LocalModule referrer, const QString &ref);
    bool start();
    bool reload();

    static void recover(const QString &pluginName,
                        const char *stepName, const v8::TryCatch &trycatch);
    static bool recover(const QString &pluginName, const QString &featureName,
                        const char *methodName, const v8::TryCatch &trycatch);

    static void initialize();
    static void teardown();
    static void reloading();
};
