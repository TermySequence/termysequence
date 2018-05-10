// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "feature.h"
#include "v8util.h"

#include <QHash>
#include <QSet>
#include <QMultiMap>
#include <map>

QT_BEGIN_NAMESPACE
class QDialog;
QT_END_NAMESPACE
class TermManager;
class ActionFeature;

//
// Monitor
//
struct ApiMonitor {
    enum Type { Timer, Attribute, Prompt };
    typedef std::pair<Type,QString> Key;

    Key key;
    PersistentObject theirs;
    PersistentObject ours;
    int timerId = 0;
    bool oneshot;
    bool canceling = false;

    ApiMonitor(Type type, QString name, LocalObject theirs);

    inline LocalObject h() { return LocalObject::New(i, ours); }
    void finalize();
};

//
// Handle
//
class ApiHandle final: public QObject
{
    Q_OBJECT

private:
    PersistentObject m_h;
    ActionFeature *m_base;
    QObject *m_target;

    typedef QMultiMap<ApiMonitor::Key,ApiMonitor*> MonitorMap;
    MonitorMap m_monitors;
    QMetaObject::Connection m_mocAttr1, m_mocAttr2;

    bool m_monitoring = false;
    unsigned m_attrmon = 0;

    LocalObject createMonitor(ApiMonitor *monitor);
    bool callMonitor(ApiMonitor *monitor, int argc, LocalValue *argv);
    MonitorMap::iterator unlinkMonitor(MonitorMap::iterator iter);
    void deleteMonitor(ApiMonitor *monitor);

private slots:
    void attributeEvent(const QString &key);
    void promptEvent();
    void cancelMonitor(ApiMonitor *monitor);

protected:
    void timerEvent(QTimerEvent *event);

signals:
    void requestCancel(ApiMonitor *monitor);

public:
    enum Type {
        Manager, Server, Term, Scrollport,
        Global, Profile, Theme, Keymap,
        Palette, Buffer, Cursor, Persistent, Region,
        Monitor, NTypes
    };
    enum Fields { Target, Handle, TypeNum, NFields };

    ApiHandle(ActionFeature *base, QObject *target, Type type, LocalObject h);
    ~ApiHandle();
    void makeWeak();
    void finalize();

    inline LocalObject h() { return LocalObject::New(i, m_h); }
    inline ActionFeature* base() const { return m_base; }

    LocalObject startTimer(LocalObject theirs, int timeout, bool oneshot);
    LocalObject startAttrmon(LocalObject theirs, const QString &key, int timeout);
    void startPrompt(LocalObject theirs, QDialog *box);
};

//
// Feature
//
class ActionFeature final: public Feature
{
private:
    PersistentFunction m_action;
    QString m_description;

    typedef std::pair<QObject*,int> HandleKey;
    std::map<HandleKey,ApiHandle*> m_handles;
    QSet<ApiHandle*> m_weaks;
    bool m_closing = false;

    static QHash<QString,ActionFeature*> s_actions;

    static void initializeApi(PersistentObjectTemplate *apitmpl);
    static void teardownApi(PersistentObjectTemplate *apitmpl);

public:
    ActionFeature(Plugin *parent, LocalFunction action, const QString &name, const QString &desc);
    ActionFeature(Plugin *parent, LocalFunction action);
    ~ActionFeature();

    inline QString description() const { return m_description; }
    inline bool closing() const { return m_closing; }

    LocalValue getHandle(ApiHandle::Type type, QObject *target);
    LocalValue getWeak(ApiHandle::Type type, QObject *target);

    static inline void registerAction(ActionFeature *feature)
    { s_actions.insert(feature->name(), feature); }
    static inline void unregisterAction(const QString &name)
    { s_actions.remove(name); }
    static inline bool hasAction(const QString &name)
    { return s_actions.contains(name); }
    static inline void reloading()
    { s_actions.clear(); }

    static QStringList customSlots();
    static bool validateName(const QString &name);
    static void invoke(TermManager *manager, const QString &name, const QStringList &args);
    static QList<QByteArray> getTip(TermManager *manager);
    static QString getDescription(const QString &name);

    static void initialize();
    static void teardown();
};
