// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "customaction.h"
#include "attrbase.h"
#include "logging.h"
#include "plugin.h"
#include "watchdog.h"
#include "base/manager.h"
#include "base/idbase.h"

#include <QRegularExpression>
#include <QInputDialog>
#include <cassert>

using namespace v8;

QHash<QString,ActionFeature*> ActionFeature::s_actions;
static const QRegularExpression s_actionRe(A("\\W"));
#define TOTD_ACTION A(".totd")

//
// Feature
//
static PersistentObjectTemplate s_apitmpl[ApiHandle::NTypes];
static PersistentString s_callbackkey;

ActionFeature::ActionFeature(Plugin *parent, LocalFunction action, const QString &name, const QString &desc) :
    Feature(ActionFeatureType, parent, name),
    m_description(desc)
{
    m_action.Reset(i, action);
}

ActionFeature::ActionFeature(Plugin *parent, LocalFunction action) :
    Feature(TipFeatureType, parent, TOTD_ACTION)
{
    m_action.Reset(i, action);
}

ActionFeature::~ActionFeature()
{
    m_closing = true;
    unregisterAction(m_name);
    forDeleteAll(m_weaks);
    for (const auto &i: m_handles)
        delete i.second;
}

QStringList
ActionFeature::customSlots()
{
    QStringList result;
    for (const QString &slot: s_actions.keys())
        if (!slot.startsWith('.'))
            result.append(CUSTOM_PREFIX + slot);
    result.sort();
    return result;
}

bool
ActionFeature::validateName(const QString &name)
{
    return !name.isEmpty() && !s_actionRe.match(name).hasMatch();
}

QString
ActionFeature::getDescription(const QString &name)
{
    const ActionFeature *base = s_actions.value(name);
    return base ? base->description() : g_mtstr;
}

LocalValue
ActionFeature::getHandle(ApiHandle::Type type, QObject *target)
{
    if (!target)
        return Undefined(i);

    const auto key = std::make_pair(target, type);
    auto k = m_handles.find(key);
    if (k != m_handles.end())
        return k->second->h();

    auto h = Local<ObjectTemplate>::New(i, s_apitmpl[type])->NewInstance();
    auto *handle = new ApiHandle(this, target, type, h);
    m_handles.emplace(key, handle);
    connect(handle, &QObject::destroyed, this, [=]{
        if (!m_closing)
            m_handles.erase(key);
    });
    return h;
}

LocalValue
ActionFeature::getWeak(ApiHandle::Type type, QObject *target)
{
    auto h = Local<ObjectTemplate>::New(i, s_apitmpl[type])->NewInstance();
    auto *handle = new ApiHandle(this, target, type, h);
    handle->makeWeak();
    m_weaks.insert(handle);
    connect(handle, &QObject::destroyed, this, [=]{
        if (!m_closing)
            m_weaks.remove(handle);
    });
    return h;
}

void
ActionFeature::invoke(TermManager *manager, const QString &name, const QStringList &args)
{
    ActionFeature *base = s_actions.value(name);
    if (!base || name.startsWith('.')) {
        qCWarning(lcPlugin) << "Unknown custom action" << name << "invoked";
        return;
    }

    g_watchdog->begin();

    HandleScope scope(i);
    auto context = base->m_plugin->context();
    Context::Scope context_scope(context);
    TryCatch trycatch(i);

    unsigned argc = 1 + args.size(), idx = 0;
    Local<Value> *argv = new Local<Value>[argc];
    argv[0] = base->getHandle(ApiHandle::Manager, manager);
    for (const QString &arg: args)
        argv[++idx] = v8str(arg);

    auto func = Local<Function>::New(i, base->m_action);
    g_watchdog->enter();
    bool rc = !func->Call(context, context->Global(), argc, argv).IsEmpty();
    g_watchdog->leave();
    delete [] argv;

    if (!rc) {
        Plugin::recover(base->m_plugin->name(), name, "action", trycatch);
    }
}

QList<QByteArray>
ActionFeature::getTip(TermManager *manager)
{
    QList<QByteArray> result;
    ActionFeature *base = s_actions.value(TOTD_ACTION);
    if (!base) {
        return result += "No tip provider installed";
    }

    g_watchdog->begin();

    HandleScope scope(i);
    auto context = base->m_plugin->context();
    Context::Scope context_scope(context);
    TryCatch trycatch(i);

    auto val = base->getHandle(ApiHandle::Manager, manager);
    auto func = Local<Function>::New(i, base->m_action);
    g_watchdog->enter();
    bool rc = func->Call(context, context->Global(), 1, &val).ToLocal(&val);
    g_watchdog->leave();

    result += "Error retrieving tip";
    if (!rc) {
        Plugin::recover(base->m_plugin->name(), A("totd"), "action", trycatch);
    }
    else if (val->IsString()) {
        result[0] = *String::Utf8Value(val);
    }
    else if (val->IsObject()) {
        auto obj = Local<Object>::Cast(val);
        if (obj->Get(context, 0).ToLocal(&val) && !val->IsNullOrUndefined()) {
            result[0] = *String::Utf8Value(val);
        }
        for (int k = 1; k < 10; ++k) {
            if (obj->Get(context, k).ToLocal(&val) && !val->IsNullOrUndefined()) {
                result.append(*String::Utf8Value(val));
            } else {
                break;
            }
        }
    }
    return result;
}

void
ActionFeature::initialize()
{
    initializeApi(s_apitmpl);
    s_callbackkey.Reset(i, v8name("callback"));
}

void
ActionFeature::teardown()
{
    s_callbackkey.Reset();
    teardownApi(s_apitmpl);
}

//
// Monitor
//
inline
ApiMonitor::ApiMonitor(Type type, QString name, LocalObject t) :
    key(type, name)
{
    theirs.Reset(i, t);
}

void
ApiMonitor::finalize()
{
    int ptri[2] = { ApiHandle::Target, ApiHandle::Handle };
    void *ptrv[2] = {};

    h()->SetAlignedPointerInInternalFields(2, ptri, ptrv);
}

//
// Handle
//
ApiHandle::ApiHandle(ActionFeature *base, QObject *target, Type type, LocalObject h) :
    QObject(target),
    m_base(base),
    m_target(target)
{
    h->SetAlignedPointerInInternalField(Target, target);
    h->SetAlignedPointerInInternalField(Handle, this);
    h->SetInternalField(TypeNum, Integer::New(i, type));
    m_h.Reset(i, h);
}

inline void
ApiHandle::finalize()
{
    m_h.Reset();
    deleteLater();
}

static void
finalizeApiHandle(const WeakCallbackInfo<ApiHandle> &data)
{
    data.GetParameter()->finalize();
}

void
ApiHandle::makeWeak()
{
    m_h.SetWeak(this, finalizeApiHandle, WeakCallbackType::kParameter);
}

ApiHandle::~ApiHandle()
{
    if (!m_base->closing()) {
        int ptri[2] = { Target, Handle };
        void *ptrv[2] = {};

        HandleScope scope(i);

        if (!m_h.IsEmpty())
            h()->SetAlignedPointerInInternalFields(2, ptri, ptrv);
        for (auto &i: qAsConst(m_monitors))
            if (!i->ours.IsEmpty())
                i->h()->SetAlignedPointerInInternalFields(2, ptri, ptrv);
    }

    forDeleteAll(m_monitors);
}

Local<Object>
ApiHandle::createMonitor(ApiMonitor *monitor)
{
    if (!m_monitoring) {
        m_monitoring = true;
        connect(this, &ApiHandle::requestCancel, this, &ApiHandle::cancelMonitor,
                Qt::QueuedConnection);
    }

    auto h = Local<ObjectTemplate>::New(i, s_apitmpl[Monitor])->NewInstance();
    h->SetAlignedPointerInInternalField(Target, monitor);
    h->SetAlignedPointerInInternalField(Handle, this);
    h->SetInternalField(TypeNum, Integer::New(i, Monitor));
    monitor->ours.Reset(i, h);
    return h;
}

inline void
ApiHandle::deleteMonitor(ApiMonitor *monitor)
{
    monitor->finalize();
    delete monitor;
}

Local<Object>
ApiHandle::startTimer(LocalObject theirs, int timeout, bool oneshot)
{
    int id = QObject::startTimer(timeout * 100);
    auto *monitor = new ApiMonitor(ApiMonitor::Timer, QString::number(id), theirs);
    monitor->timerId = id;
    monitor->oneshot = oneshot;
    m_monitors.insert(monitor->key, monitor);
    return createMonitor(monitor);
}

Local<Object>
ApiHandle::startAttrmon(LocalObject theirs, const QString &name, int timeout)
{
    if (!m_mocAttr1) {
        IdBase *target = static_cast<IdBase*>(m_target);
        m_mocAttr1 = connect(target, &IdBase::attributeChanged, this, &ApiHandle::attributeEvent,
                             Qt::QueuedConnection);
        m_mocAttr2 = connect(target, &IdBase::attributeRemoved, this, &ApiHandle::attributeEvent,
                             Qt::QueuedConnection);
    }

    auto *monitor = new ApiMonitor(ApiMonitor::Attribute, name, theirs);
    if (timeout > 0)
        monitor->timerId = QObject::startTimer(timeout * 100);
    m_monitors.insert(monitor->key, monitor);
    ++m_attrmon;
    return createMonitor(monitor);
}

void
ApiHandle::startPrompt(LocalObject theirs, QDialog *box)
{
    auto *monitor = new ApiMonitor(ApiMonitor::Prompt, g_mtstr, theirs);
    connect(box, &QDialog::accepted, this, &ApiHandle::promptEvent);
    connect(box, &QObject::destroyed, this, [=]{
        m_monitors.remove(monitor->key);
        delete monitor;
    });
    m_monitors.insert(monitor->key, monitor);
}

ApiHandle::MonitorMap::iterator
ApiHandle::unlinkMonitor(MonitorMap::iterator k)
{
    auto *monitor = *k;

    if (monitor->timerId)
        killTimer(monitor->timerId);

    if (monitor->key.first == ApiMonitor::Attribute && --m_attrmon == 0) {
        disconnect(m_mocAttr1);
        disconnect(m_mocAttr2);
    }

    if (!monitor->canceling) {
        deleteMonitor(monitor);
    }
    return m_monitors.erase(k);
}

void
ApiHandle::cancelMonitor(ApiMonitor *monitor)
{
    HandleScope scope(i);

    for (auto k = m_monitors.find(monitor->key), l = m_monitors.end(); k != l; ++k)
        if (*k == monitor) {
            unlinkMonitor(k);
            break;
        }

    deleteMonitor(monitor);
}

bool
ApiHandle::callMonitor(ApiMonitor *monitor, int argc, LocalValue *argv)
{
    g_watchdog->begin();

    auto context = m_base->plugin()->context();
    Context::Scope context_scope(context);
    TryCatch trycatch(i);

    auto theirs = Local<Object>::New(i, monitor->theirs);

    // Callback
    Local<Value> val = Local<String>::New(i, s_callbackkey);
    if (theirs->Get(context, val).ToLocal(&val) && val->IsFunction())
    {
        auto func = Local<Function>::Cast(val);
        g_watchdog->enter();
        bool rc = func->Call(context, theirs, argc, argv).ToLocal(&val);
        g_watchdog->leave();

        if (!rc) {
            Plugin::recover(m_base->plugin()->name(), m_base->name(), "callback", trycatch);
        }
        else if (val->IsTrue() || val->IsUndefined()) {
            return true;
        }
    }
    return false;
}

void
ApiHandle::timerEvent(QTimerEvent *event)
{
    int id = event->timerId();
    ApiMonitor::Key key = std::make_pair(ApiMonitor::Timer, QString::number(id));
    auto k = m_monitors.find(key), l = m_monitors.end();

    HandleScope scope(i);

    if (k != l) {
        Local<Value> arg = h();

        // Callback
        if (!callMonitor(*k, 1, &arg) || (*k)->oneshot) {
            // Cancel
            unlinkMonitor(k);
        }
    } else {
        // Expiring attribute monitor
        for (k = m_monitors.begin(); k != l; ++k)
            if ((*k)->timerId == id) {
                unlinkMonitor(k);
                break;
            }
    }
}

void
ApiHandle::attributeEvent(const QString &name)
{
    ApiMonitor::Key key = std::make_pair(ApiMonitor::Attribute, name);
    auto k = m_monitors.constFind(key);
    if (k == m_monitors.cend())
        return;

    HandleScope outer(i);

    IdBase *target = static_cast<IdBase*>(m_target);
    Local<Value> val;
    auto v = target->attributes().constFind(name);
    if (v != target->attributes().cend())
        val = v8str(*v);
    else
        val = Undefined(i);

    const int argc = 3;
    Local<Value> argv[argc] = { h(), val, v8str(name) };

    for (auto k = m_monitors.find(key); k != m_monitors.end() && k.key() == key; ) {
        // Callback
        HandleScope inner(i);
        if (callMonitor(*k, argc, argv)) {
            ++k;
        } else {
            // Cancel
            k = unlinkMonitor(k);
        }
    }
}

void
ApiHandle::promptEvent()
{
    auto data = static_cast<QInputDialog*>(sender())->textValue().toUtf8();

    ApiMonitor::Key key = std::make_pair(ApiMonitor::Prompt, g_mtstr);
    ApiMonitor *monitor = m_monitors.value(key);

    HandleScope scope(i);
    Local<String> val;

    if (v8maybestr(data, &val)) {
        const int argc = 2;
        Local<Value> argv[argc] = { h(), val };
        // Callback
        callMonitor(monitor, argc, argv);
    }
}
