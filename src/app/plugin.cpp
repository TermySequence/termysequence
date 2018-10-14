// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "plugin.h"
#include "attrbase.h"
#include "customaction.h"
#include "logging.h"
#include "watchdog.h"
#include "base/sembase.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "os/time.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>

#define API_MAJOR_VERSION 1
#define API_MINOR_VERSION 3

using namespace v8;

#define context_thiz \
    auto *thiz = \
    static_cast<Plugin*>(i->GetCurrentContext()->GetAlignedPointerFromEmbedderData(0));

static int64_t s_basetime;
static PersistentObjectTemplate s_globtmpl;
static PersistentObjectTemplate s_contmpl;
static PersistentObjectTemplate s_filetmpl;

unsigned
qHash(const PersistentModule &pm)
{
    return LocalModule::New(i, pm)->GetIdentityHash();
}

//
// Global Api
//
static void
cbGetProp(Local<String> key, const PropertyCallbackInfo<Value> &args)
{
    context_thiz;
    v8ret(v8str(thiz->property(*String::Utf8Value(key)).toByteArray()));
}

static void
cbSetProp(Local<String> key, Local<Value> value, const PropertyCallbackInfo<void> &)
{
    context_thiz;
    QString str = *String::Utf8Value(value);
    thiz->setProperty(*String::Utf8Value(key), str);
}

static void
cbLog(const FunctionCallbackInfo<Value> &args)
{
    context_thiz;
    String::Utf8Value utf8(args[0]);

    switch (args.Data()->Int32Value()) {
    case QtWarningMsg:
        qCWarning(lcPlugin, "Plugin [%s]: %s", pr(thiz->name()), *utf8);
        break;
    default:
        qCInfo(lcPlugin, "Plugin [%s]: %s", pr(thiz->name()), *utf8);
        break;
    }
}

static void
cbNow(const FunctionCallbackInfo<Value> &args)
{
    v8ret(Integer::New(i, osModtime(s_basetime)));
}

static void
cbHtmlEscape(const FunctionCallbackInfo<Value> &args)
{
    String::Utf8Value utf8(args[0]);
    v8ret(v8str(QString(*utf8).toHtmlEscaped()));
}

static void
cbRegisterSemanticParser(const FunctionCallbackInfo<Value> &args)
{
    context_thiz;

    if (thiz->loaded() || args[0]->Int32Value() != 1) {
        v8throw(v8literal("unsupported API version requested"));
        return;
    }
    if (!args[2]->IsFunction()) {
        v8throw(v8literal("invalid argument"));
        return;
    }

    QString name(*String::Utf8Value(args[1]));
    unsigned typen = args[3]->Uint32Value();

    if (typen > SemanticFeature::NSemanticFeatures) {
        qCWarning(lcPlugin) << "Unsupported type" << typen << "parser" << name;
        return;
    }

    auto type = (SemanticFeature::ParserType)typen;
    auto matcher = Local<Function>::Cast(args[2]);
    auto feature = new SemanticFeature(thiz, name, matcher, type);
    thiz->addFeature(feature);

    qCInfo(lcPlugin) << "Registered type" << typen << "parser" << name;
}

static void
cbRegisterCustomAction(const FunctionCallbackInfo<Value> &args)
{
    context_thiz;

    if (thiz->loaded() || args[0]->Int32Value() != 1) {
        v8throw(v8literal("unsupported API version requested"));
        return;
    }

    QString name(*String::Utf8Value(args[1]));
    QString desc(*String::Utf8Value(args[3]));

    if (!args[2]->IsFunction() || !ActionFeature::validateName(name)) {
        v8throw(v8literal("invalid argument"));
        return;
    }
    if (ActionFeature::hasAction(name)) {
        v8throw(v8str(L("action '%1' already registered").arg(name)));
        return;
    }
    if (!args[3]->IsString()) {
        desc = thiz->description();
    }

    auto action = Local<Function>::Cast(args[2]);
    auto feature = new ActionFeature(thiz, action, name, desc);
    thiz->addFeature(feature);

    qCInfo(lcPlugin) << "Registered custom action" << name;
}

static void
cbRegisterTipProvider(const FunctionCallbackInfo<Value> &args)
{
    context_thiz;

    if (thiz->loaded() || args[0]->Int32Value() != 1) {
        v8throw(v8literal("unsupported API version requested"));
        return;
    }
    if (!args[1]->IsFunction()) {
        v8throw(v8literal("invalid argument"));
        return;
    }

    auto action = Local<Function>::Cast(args[1]);
    auto feature = new ActionFeature(thiz, action);
    thiz->addFeature(feature);

    qCInfo(lcPlugin) << "Registered tip provider";
}

//
// File Api
//
static void
cbCreateFile(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);

    QString name(*String::Utf8Value(args[0]));
    if (!args[0]->IsString() || name.isEmpty() || name.contains('/')) {
        v8throw(v8literal("invalid argument"));
        return;
    }

    QDir dir(g_global->downloadLocation());
    QByteArray path = dir.filePath(name).toUtf8();
    int fd = open(path.data(), O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC, 0644);
    if (fd == -1) {
        v8throw(v8str(L("%1: %2").arg(path, strerror(errno))));
        return;
    }

    auto handle = Local<ObjectTemplate>::New(i, s_filetmpl)->NewInstance();
    handle->SetInternalField(0, Integer::New(i, fd));
    handle->SetInternalField(1, v8str(path));
    v8ret(handle);
}

static void
cbGetFilePath(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    v8ret(args.This()->GetInternalField(1));
}

static void
cbPrintFile(const FunctionCallbackInfo<Value> &args)
{
    String::Utf8Value utf8(args[0]);
    write(args.This()->GetInternalField(0)->Int32Value(), *utf8, utf8.length());
}

static void
cbCloseFile(const FunctionCallbackInfo<Value> &args)
{
    close(args.This()->GetInternalField(0)->Int32Value());
    args.This()->SetInternalField(0, Integer::New(i, -1));
}

//
// Plugin
//
Plugin::Plugin(const QString &path, const QString &name) :
    m_path(path),
    m_name(name)
{
}

bool
Plugin::loadModule(const QString &path, Local<Module> &result, bool isroot)
{
    qCInfo(lcPlugin) << (isroot ? "Loading plugin" : "Loading dependency") << path;
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        v8throw(v8str(L("Failed to open %1 for reading: %2").arg(path, file.errorString())));
        return false;
    }

    // Read into string
    Local<String> text;
    if (!v8maybestr(file.readAll(), &text)) {
        v8throw(v8str(L("Failed to load %1: %2").arg(path, strerror(EFBIG))));
        return false;
    }

    // Compile
    ScriptOrigin origin(v8str(path),
                        Local<Integer>(), Local<Integer>(), Local<Boolean>(),
                        Local<Integer>(), Local<Value>(), Local<Boolean>(),
                        Local<Boolean>(), True(i)); // Best API ever
    ScriptCompiler::Source source(text, origin);
    g_watchdog->enter();
    bool rc = ScriptCompiler::CompileModule(i, &source).ToLocal(&result);
    g_watchdog->leave();
    if (!rc) {
        return false;
    }

    // Save
    QString dir = QFileInfo(path).path() + '/';
    PersistentModule pm(i, result);
    m_modules[path] = pm;
    m_modlocs[pm] = dir;

    // Recurse
    for (int k = 0, n = result->GetModuleRequestsLength(); k < n; ++k) {
        auto path = QDir::cleanPath(dir + *String::Utf8Value(result->GetModuleRequest(k)));
        Local<Module> unused;
        if (!m_modules.contains(path) && !loadModule(path, unused, false))
            return false;
    }
    return true;
}

LocalModule
Plugin::getModule(LocalModule referrer, const QString &ref)
{
    PersistentModule pm(i, referrer);
    auto path = QDir::cleanPath(m_modlocs[pm] + ref);
    return LocalModule::New(i, m_modules[path]);
}

static MaybeLocal<Module>
cbResolve(LocalContext context, LocalString ref, LocalModule referrer)
{
    context_thiz;
    return thiz->getModule(referrer, *String::Utf8Value(ref));
}

bool
Plugin::start()
{
    HandleScope scope(i);
    auto context = Context::New(i, nullptr, Local<ObjectTemplate>::New(i, s_globtmpl));
    context->SetAlignedPointerInEmbedderData(0, this);
    Context::Scope context_scope(context);

    // Setup
    MaybeLocal<Object> console;
    console = Local<ObjectTemplate>::New(i, s_contmpl)->NewInstance(context);
    context->Global()->Set(v8name("console"), console.ToLocalChecked());

    m_context.Reset(i, context);
    TryCatch trycatch(i);
    m_loaded = false;
    g_watchdog->begin();

    // Load
    Local<Module> root;
    if (!loadModule(m_path, root, true)) {
        recover(m_name, "compile", trycatch);
        return true;
    }

    // Execute
    MaybeLocal<Value> result;
    g_watchdog->enter();
    if (root->InstantiateModule(context, cbResolve).FromMaybe(false))
        result = root->Evaluate(context);
    g_watchdog->leave();
    if (result.IsEmpty()) {
        recover(m_name, "load", trycatch);
        return true;
    }

    return m_loaded = !m_features.isEmpty();
}

bool
Plugin::reload()
{
    forDeleteAll(m_features);
    m_features.clear();
    m_semantics.clear();
    m_modlocs.clear();
    m_modules.clear();
    return start();
}

void
Plugin::addFeature(Feature *feature)
{
    switch (feature->type()) {
    case Feature::SemanticFeatureType:
        m_semantics.append(static_cast<SemanticFeature*>(feature));
        break;
    case Feature::ActionFeatureType:
    case Feature::TipFeatureType:
        ActionFeature::registerAction(static_cast<ActionFeature*>(feature));
        break;
    };

    m_features.append(feature);
}

void
Plugin::unloadFeature(const QString &featureName)
{
    for (auto feature: m_features)
        if (feature->name() == featureName)
        {
            switch (feature->type()) {
            case Feature::SemanticFeatureType:
                m_semantics.removeOne(static_cast<SemanticFeature*>(feature));
                break;
            case Feature::ActionFeatureType:
            case Feature::TipFeatureType:
                // ActionFeature unregisters in destructor
                break;
            default:
                break;
            };

            m_features.removeOne(feature);
            emit featureUnloaded(feature);
            delete feature;
            break;
        }
}

void
Plugin::setName(const QString &name)
{
    if (!m_loaded)
        m_name = name;
}

void
Plugin::setDescription(const QString &description)
{
    if (!m_loaded)
        m_description = description;
}

void
Plugin::setVersion(const QString &version)
{
    if (!m_loaded)
        m_version = version;
}

void
Plugin::initialize()
{
    if (!i)
        return;

    g_watchdog = new WatchdogController;
    osBasetime(&s_basetime);

    Local<FunctionTemplate> func;
    HandleScope scope(i);

    auto filetmpl = ObjectTemplate::New(i);
    filetmpl->SetInternalFieldCount(2);
    filetmpl->SetAccessor(v8name("path"), cbGetFilePath);
    filetmpl->v8method("print", cbPrintFile);
    filetmpl->v8method("close", cbCloseFile);
    s_filetmpl.Reset(i, filetmpl);

    auto contmpl = ObjectTemplate::New(i);
    contmpl->v8switch("warn", cbLog, QtWarningMsg);
    contmpl->v8switch1("info", cbLog, QtInfoMsg, func);
    contmpl->v8constant("log", func);
    s_contmpl.Reset(i, contmpl);

    auto apitmpl = ObjectTemplate::New(i);
    apitmpl->v8constant("majorVersion", Integer::New(i, API_MAJOR_VERSION));
    apitmpl->v8constant("minorVersion", Integer::New(i, API_MINOR_VERSION));
    apitmpl->v8constant("installPrefix", v8literal(PREFIX));
    apitmpl->SetAccessor(v8name("pluginName"), cbGetProp, cbSetProp);
    apitmpl->SetAccessor(v8name("pluginDescription"), cbGetProp, cbSetProp);
    apitmpl->SetAccessor(v8name("pluginVersion"), cbGetProp, cbSetProp);
    apitmpl->v8method("now", cbNow);
    apitmpl->v8method("htmlEscape", cbHtmlEscape);
    apitmpl->v8method("createOutputFile", cbCreateFile);
    apitmpl->v8method("registerSemanticParser", cbRegisterSemanticParser);
    apitmpl->v8method("registerCustomAction", cbRegisterCustomAction);
    apitmpl->v8method("registerTipProvider", cbRegisterTipProvider);

    auto globtmpl = ObjectTemplate::New(i);
    globtmpl->Set(v8name("plugin"), apitmpl, v8::ReadOnly);
    s_globtmpl.Reset(i, globtmpl);

    SemanticFeature::initialize();
    ActionFeature::initialize();
}

void
Plugin::teardown()
{
    if (i) {
        SemanticFeature::teardown();
        ActionFeature::teardown();

        s_globtmpl.Reset();
        s_contmpl.Reset();
        s_filetmpl.Reset();

        delete g_watchdog;
    }
}

void
Plugin::reloading()
{
    if (i) {
        ActionFeature::reloading();
    }
}

static QString
recoverString(const v8::TryCatch &trycatch)
{
    QString str(*String::Utf8Value(trycatch.Exception()));
    auto message = trycatch.Message();
    auto context = i->GetCurrentContext();
    int line, column;

    if (!message.IsEmpty() &&
        message->GetLineNumber(context).To(&line) &&
        message->GetStartColumn(context).To(&column))
        str += L(" (at line %1 column %2)").arg(line).arg(column);

    return str;
}

void
Plugin::recover(const QString &pluginName, const char *stepName,
                const v8::TryCatch &trycatch)
{
    auto pluginStr = pluginName.toUtf8();

    if (trycatch.HasTerminated()) {
        qCCritical(lcPlugin, "Plugin [%s] failed to %s: timeout exceeded, execution terminated",
                   pluginStr.data(), stepName);

        i->CancelTerminateExecution();
    }
    else {
        qCWarning(lcPlugin, "Plugin [%s]: exception thrown during %s: %s",
                  pluginStr.data(), stepName,
                  pr(recoverString(trycatch)));
    }
}

bool
Plugin::recover(const QString &pluginName, const QString &featureName,
                const char *methodName, const v8::TryCatch &trycatch)
{
    auto pluginStr = pluginName.toUtf8();
    auto featureStr = featureName.toUtf8();

    if (trycatch.HasTerminated()) {
        qCCritical(lcPlugin, "Plugin [%s]: timeout exceeded in %s.%s(), execution terminated",
                   pluginStr.data(), featureStr.data(), methodName);
        qCCritical(lcPlugin, "Plugin [%s]: unloading feature %s (possible infinite loop)",
                   pluginStr.data(), featureStr.data());

        g_settings->unloadFeature(pluginName, featureName);
        i->CancelTerminateExecution();
        return false;
    }
    else {
        qCWarning(lcPlugin, "Plugin [%s]: exception thrown in %s.%s(): %s",
                  pluginStr.data(), featureStr.data(), methodName,
                  pr(recoverString(trycatch)));
        return true;
    }
}
