// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "rule.h"

#include <QObject>
#include <QMultiMap>
#include <QKeyEvent>
#include <QFile>

class TermManager;
class InfoAnimation;

class TermKeymap final: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name)

private:
    KeymapRuleset *const m_mainRuleset;
    KeymapRuleset *m_currentRuleset;
    TermKeymap *m_parent = nullptr;
    unsigned m_enabledFeatures, m_disabledFeatures;

    unsigned m_features;
    bool m_comboInProgress = false;

    int m_row = -1;
    InfoAnimation *m_animation = nullptr;

    QString m_path;
    QString m_name;
    QString m_description;
    int m_size = 0;
    bool m_reserved;
    bool m_loaded = false;
    bool m_error = false;
    int m_keymapCount = 1, m_profileCount = 0;

    QMultiMap<QString,TermShortcut> m_shortcuts;
    TermShortcut m_digraphShortcut;

    void parseDefault();
    bool parseFile(QFile &file);
    bool parseRule(const QStringList &parts, bool special, int lineno);
    void parseFeature(const QStringList &parts, int lineno);
    bool parseInheritance(const QString &target, int lineno);
    void load();

    QByteArray translateCombo(TermManager *manager, const QKeyEvent *event, Tsq::TermFlags state);

    void constructShortcut(const KeymapRule *rule, TermShortcut &result);
    void addShortcut(KeymapRule *rule, const TermShortcut *parent);

signals:
    void comboFinished(int code);
    void rulesChanged();
    void rulesReloaded();
    void featuresChanged();

private slots:
    void handleParentDestroyed();
    void computeFeatures();

public:
    TermKeymap(const QString &name, const QString &path, bool reserved = false);
    ~TermKeymap();

    void save();
    void copy(TermKeymap *other) const;
    void setRules(const KeymapRulelist *rules);
    void setFeatures(unsigned enabled, unsigned disabled);
    void setDescription(const QString &description);
    void resetToDefaults();
    void activate();
    void reload();

    inline const QString& name() const { return m_name; }
    inline const TermKeymap* parent() const { return m_parent; }
    inline const QString& description() const { return m_description; }
    inline int size() const { return m_size; }
    inline const KeymapRuleset* rules() const { return m_mainRuleset; }
    inline unsigned enabledFeatures() const { return m_enabledFeatures; }
    inline unsigned disabledFeatures() const { return m_disabledFeatures; }

    inline bool reserved() const { return m_reserved; }
    inline bool error() const { return m_error; }
    inline int keymapCount() const { return m_keymapCount; }
    inline int profileCount() const { return m_profileCount; }

    inline int row() const { return m_row; }
    inline InfoAnimation* animation() const { return m_animation; }
    inline void setRow(int row) { m_row = row; }
    void adjustRow(int delta);
    InfoAnimation *createAnimation();

    void takeKeymapReference(int count=1);
    void putKeymapReference(int count=1);
    void takeProfileReference();
    void putProfileReference();

    void reparent(TermKeymap *parent);

    QByteArray translate(TermManager *manager, const QKeyEvent *event, Tsq::TermFlags state);
    void reset();

    bool lookupShortcut(const QString &slot, const Tsq::TermFlags flags,
                        TermShortcut *result) const;
};
