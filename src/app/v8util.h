// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <v8.h>
#include <QString>

extern v8::Isolate *i;

typedef v8::Local<v8::Value> LocalValue;
typedef v8::Local<v8::Context> LocalContext;
typedef v8::Local<v8::Function> LocalFunction;
typedef v8::Local<v8::Object> LocalObject;
typedef v8::Local<v8::FunctionTemplate> LocalFunctionTemplate;
typedef v8::Local<v8::ObjectTemplate> LocalObjectTemplate;
typedef v8::Local<v8::String> LocalString;
typedef v8::Local<v8::Private> LocalPrivate;
typedef v8::Local<v8::Module> LocalModule;

typedef v8::Persistent<v8::Context,v8::CopyablePersistentTraits<v8::Context>> PersistentContext;
typedef v8::Persistent<v8::Function,v8::CopyablePersistentTraits<v8::Function>> PersistentFunction;
typedef v8::Persistent<v8::Object,v8::CopyablePersistentTraits<v8::Object>> PersistentObject;
typedef v8::Persistent<v8::FunctionTemplate,v8::CopyablePersistentTraits<v8::FunctionTemplate>> PersistentFunctionTemplate;
typedef v8::Persistent<v8::ObjectTemplate,v8::CopyablePersistentTraits<v8::ObjectTemplate>> PersistentObjectTemplate;
typedef v8::Persistent<v8::String,v8::CopyablePersistentTraits<v8::String>> PersistentString;
typedef v8::Persistent<v8::Private,v8::CopyablePersistentTraits<v8::Private>> PersistentPrivate;
typedef v8::Persistent<v8::Module,v8::CopyablePersistentTraits<v8::Module>> PersistentModule;

extern unsigned qHash(const PersistentModule &pm);

#define v8name(x) String::NewFromUtf8(i, x, NewStringType::kInternalized, sizeof(x) - 1).ToLocalChecked()
#define v8literal(x) v8::String::NewFromUtf8(i, x, v8::NewStringType::kNormal, sizeof(x) - 1).ToLocalChecked()
#define v8constant(name, val) Set(v8name(name), val, v8::PropertyAttribute::ReadOnly)
#define v8method(name, cb) Set(v8name(name), FunctionTemplate::New(i, cb), v8::PropertyAttribute::ReadOnly);
#define v8switch(name, cb, num) Set(v8name(name), FunctionTemplate::New(i, cb, Integer::New(i, num)), v8::PropertyAttribute::ReadOnly);
#define v8switch1(name, cb, num, var) Set(v8name(name), var = FunctionTemplate::New(i, cb, Integer::New(i, num)), v8::PropertyAttribute::ReadOnly);
#define v8switchget(name, get, num) SetAccessor(v8name(name), get, 0, Integer::New(i, num))
#define v8switchset(name, get, set, num) SetAccessor(v8name(name), get, set, Integer::New(i, num))
#define v8throw(val) i->ThrowException(v8::Exception::Error(val))
#define v8ret(expr) args.GetReturnValue().Set(expr)

static inline bool v8maybestr(const QByteArray &ba, LocalString *out) {
    return v8::String::NewFromUtf8(i, ba.data(), v8::NewStringType::kNormal, ba.size()).ToLocal(out);
}
static inline LocalString v8str(const std::string &str) {
    return v8::String::NewFromUtf8(i, str.data(), v8::NewStringType::kNormal, str.size()).ToLocalChecked();
}
static inline LocalString v8str(const QByteArray &ba) {
    return v8::String::NewFromUtf8(i, ba.data(), v8::NewStringType::kNormal, ba.size()).ToLocalChecked();
}
static inline LocalString v8str(const QString &str) {
    return v8str(str.toUtf8());
}
static inline LocalString v8str(const QStringRef &str) {
    return v8str(str.toUtf8());
}

#define OBJPROP_V8_ERROR      "v8DisabledError"
#define OBJPROP_V8_SYSPLUGINS "v8LoadSystemPlugins"
