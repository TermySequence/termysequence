// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "customaction.h"
#include "attr.h"
#include "messagebox.h"
#include "base/listener.h"
#include "base/manager.h"
#include "base/server.h"
#include "base/term.h"
#include "base/scrollport.h"
#include "base/stack.h"
#include "base/buffers.h"
#include "base/screen.h"
#include "base/selection.h"
#include "base/mark.h"
#include "base/sembase.h"
#include "base/semflash.h"
#include "base/arrange.h"
#include "base/termwidget.h"
#include "settings/settings.h"
#include "settings/global.h"
#include "settings/keymap.h"
#include "settings/profile.h"
#include "settings/servinfo.h"
#include "settings/theme.h"
#include "lib/grapheme.h"
#include "lib/utf8.h"

#include <QApplication>
#include <QClipboard>

#define TR_TITLE1 TL("window-title", "Input")

using namespace v8;

static PersistentPrivate s_rowPriv;
static PersistentPrivate s_posPriv;
static PersistentPrivate s_row2Priv;
static PersistentPrivate s_pos2Priv;
static PersistentPrivate s_idPriv;
static PersistentPrivate s_parentPriv;
static PersistentPrivate s_typePriv;
static PersistentPrivate s_flagsPriv;

//
// Macros
//
#define check_pointer(ptr) \
    if (!ptr) { \
        v8throw(v8literal("invalid handle")); \
        return; \
    }

#define declare_target_checked(varname, type) \
    auto *varname = \
    static_cast<type*>(args.This()->GetAlignedPointerFromInternalField(ApiHandle::Target)); \
    check_pointer(varname)

#define declare_target(varname, type) \
    auto *varname = \
    static_cast<type*>(args.This()->GetAlignedPointerFromInternalField(ApiHandle::Target))

#define declare_handle_checked \
    auto *handle = \
    static_cast<ApiHandle*>(args.This()->GetAlignedPointerFromInternalField(ApiHandle::Handle)); \
    check_pointer(handle)

#define declare_handle \
    auto *handle = \
    static_cast<ApiHandle*>(args.This()->GetAlignedPointerFromInternalField(ApiHandle::Handle))

#define get_handle(type, expr) handle->base()->getHandle(ApiHandle::type, expr)
#define get_weak(type, expr) handle->base()->getWeak(ApiHandle::type, expr)
#define is_handle(expr) ((expr)->InternalFieldCount() == ApiHandle::NFields)

#define declare_priv(priv) auto priv = Local<Private>::New(i, s_ ## priv)
#define get_priv(priv) GetPrivate(context, priv).ToLocalChecked()
#define set_priv(priv, expr) SetPrivate(context, priv, expr)

static void *
getArg(Local<Value> v, Local<Object> &o, ApiHandle::Type type)
{
    return v->IsObject() && is_handle(o = Local<Object>::Cast(v)) &&
        o->GetInternalField(ApiHandle::TypeNum)->Int32Value() == type ?
        o->GetAlignedPointerFromInternalField(ApiHandle::Target) :
        nullptr;
}

#define declare_arg(varname, objtype, type, expr, hname) \
    Local<Object> hname; \
    auto *varname = static_cast<objtype*>(getArg(expr, hname, ApiHandle::type));

#define declare_arg_checked(varname, objtype, type, expr, hname) \
    Local<Object> hname; \
    auto *varname = static_cast<objtype*>(getArg(expr, hname, ApiHandle::type)); \
    check_pointer(varname)

enum CursorMoveOp {
    ByRows, ToRow, ByOffset, ToOffset, ByColumns, ToColumn, ByChars, ToChar,
    ToEnd, ToNextJob, ToPrevJob, ToNextNote, ToPrevNote,
    ToFirstRow, ToLastRow, ToScreenStart, ToScreenEnd, ToScreenCursor,
};
enum CursorProp {
    CursorRow, CursorOffset, CursorColumn, CursorChar,
    CursorColumns, CursorChars, CursorContinuation, CursorFlags,
    CursorText, CursorTextBefore, CursorTextAfter,
};
enum RegionProp {
    RegionId, RegionFlags,
};

//
// Position helpers
//
static column_t
posByJavascript(TermBuffers *buffers, size_t row, size_t jspos)
{
    const CellRow &crow = buffers->safeRow(row);
    size_t curpos = 0;
    const char *start, *next, *end;
    next = start = crow.str.data(), end = start + crow.str.size();

    while (curpos < jspos && next != end) {
        curpos += 1 + (utf8::unchecked::next(next) > 0xffff);
    }

    auto *unicoding = buffers->term()->unicoding();
    column_t pos = 0;

    while (start != next) {
        unicoding->next(start, next);
        ++pos;
    }
    return pos;
}

static size_t
javascriptByPos(TermBuffers *buffers, size_t row, column_t pos)
{
    const CellRow &crow = buffers->safeRow(row);
    const char *start, *next, *end;
    next = start = crow.str.data(), end = start + crow.str.size();
    auto *unicoding = buffers->term()->unicoding();

    for (column_t i = 0; i < pos && next != end; ++i) {
        unicoding->next(next, end);
    }

    size_t jspos = 0;

    while (start != next) {
        jspos += 1 + (utf8::unchecked::next(start) > 0xffff);
    }
    return jspos;
}

static size_t
ptrByPos(TermBuffers *buffers, const std::string &str, column_t pos)
{
    const char *start, *next, *end;
    next = start = str.data(), end = start + str.size();
    auto *unicoding = buffers->term()->unicoding();

    for (column_t i = 0; i < pos && next != end; ++i) {
        unicoding->next(next, end);
    }
    return next - start;
}

static unsigned
xSize(TermBuffers *buffers, const std::string &str)
{
    Tsq::CategoryWalk cbf(buffers->term()->unicoding(), str);
    Tsq::CellFlags flags;
    column_t size, total = 0;

    while ((size = cbf.next(flags))) {
        total += size * (1 + !!(flags & Tsq::DblWidthChar));
    }

    return total;
}

static unsigned
posSize(TermBuffers *buffers, const std::string &str)
{
    Tsq::GraphemeWalk tbf(buffers->term()->unicoding(), str);
    column_t total;
    for (total = 0; tbf.next(); ++total);
    return total;
}

static inline void
cursorPosHelper(LocalObject cursor, LocalContext context, TermBuffers *buffers,
                size_t row, column_t pos, LocalPrivate posPriv, LocalPrivate pos2Priv)
{
    Local<Value> val;
    cursor->set_priv(posPriv, val = Integer::NewFromUnsigned(i, pos));
    if (pos != 0) {
        val = Integer::NewFromUnsigned(i, javascriptByPos(buffers, row, pos));
    }
    cursor->set_priv(pos2Priv, val);
}

//
// Common Api
//
static void
cbIsValid(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_handle;
    v8ret(Boolean::New(i, handle));
}

static void
cbGetProp(Local<String> key, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(target, QObject);
    QVariant value = target->property(*String::Utf8Value(key));

    switch (args.Data()->Int32Value()) {
    case QVariant::Bool:
        v8ret(Boolean::New(i, value.toBool()));
        break;
    case QVariant::Int:
        v8ret(Integer::New(i, value.toInt()));
        break;
    default:
        v8ret(v8str(value.toByteArray()));
        break;
    }
}

static void
cbSetProp(Local<String> key, Local<Value> value, const PropertyCallbackInfo<void> &args)
{
    HandleScope scope(i);
    declare_target_checked(target, IdBase);
    QString str = *String::Utf8Value(value);
    target->setProperty(*String::Utf8Value(key), str);
}

static void
cbGetPrivateData(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    if (args[0]->IsString()) {
        auto context = i->GetCurrentContext();
        auto name = Private::ForApi(i, Local<String>::Cast(args[0]));
        Local<Value> result;
        if (args.This()->GetPrivate(context, name).ToLocal(&result))
            v8ret(result);
    }
}

static void
cbSetPrivateData(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    if (args[0]->IsString()) {
        auto context = i->GetCurrentContext();
        auto name = Private::ForApi(i, Local<String>::Cast(args[0]));
        args.This()->set_priv(name, args[1]);
    }
}

static void
cbSetInterval(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_handle_checked;

    int timeout = args[1]->Int32Value();
    if (args[0]->IsObject() && args[1]->IsNumber() && timeout > 0) {
        v8ret(handle->startTimer(Local<Object>::Cast(args[0]), timeout, false));
    } else {
        v8throw(v8literal("invalid argument"));
    }
}

//
// Manager Api
//
static void
managerInvoke(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    QString actionName(*String::Utf8Value(args[0]));
    if (!args[0]->IsString() || actionName.isEmpty()) {
        v8throw(v8literal("invalid argument"));
        return;
    }

    QStringList slot(actionName);

    for (int i = 1, n = args.Length(); i < n; ++i)
        slot.append(*String::Utf8Value(args[i]));

    declare_target_checked(manager, TermManager);
    emit manager->invokeRequest(slot.join('|'), false);
}

static void
managerNotifySend(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    QString summary, body;

    if (!args[0]->IsNullOrUndefined())
        summary = *String::Utf8Value(args[0]);
    if (!args[1]->IsNullOrUndefined())
        body = *String::Utf8Value(args[1]);

    declare_target_checked(manager, TermManager);
    manager->actionNotifySend(summary, body);
}

static void
managerListServers(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(manager, TermManager);
    declare_handle;

    const auto &servers = manager->servers();
    int n = servers.size();
    Local<Array> arr = Array::New(i, n);
    auto context = i->GetCurrentContext();

    for (int k = 0; k < n; ++k) {
        arr->Set(context, k, get_handle(Server, servers[k])).ToChecked();
    }
    v8ret(arr);
}

static void
managerGetServerById(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(order, TermOrder);
    declare_handle;
    Tsq::Uuid id(std::string(*String::Utf8Value(args[0])));
    v8ret(get_handle(Server, order->lookupServer(id)));
}

static void
managerGetActiveServer(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(manager, TermManager);
    declare_handle;
    v8ret(get_handle(Server, manager->activeServer()));
}

static void
managerGetLocalServer(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_handle_checked;
    v8ret(get_handle(Server, g_listener->localServer()));
}

static void
managerListTerminals(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(manager, TermManager);
    declare_handle;

    const auto &terms = manager->terms();
    int n = terms.size();
    Local<Array> arr = Array::New(i, n);
    auto context = i->GetCurrentContext();

    for (int k = 0; k < n; ++k) {
        arr->Set(context, k, get_handle(Term, terms[k])).ToChecked();
    }
    v8ret(arr);
}

static void
managerGetTerminalById(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(order, TermOrder);
    declare_handle;
    Tsq::Uuid id(std::string(*String::Utf8Value(args[0])));
    v8ret(get_handle(Term, order->lookupTerm(id)));
}

static void
managerGetActiveTerminal(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(manager, TermManager);
    declare_handle;
    v8ret(get_handle(Term, manager->activeTerm()));
}

static void
managerGetActiveScrollport(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(manager, TermManager);
    declare_handle;
    v8ret(get_handle(Scrollport, manager->activeScrollport()));
}

static void
managerGetGlobalSettings(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_handle_checked;
    v8ret(get_handle(Global, g_global));
}

static void
managerGetDefaultProfile(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_handle_checked;
    v8ret(get_handle(Profile, g_settings->defaultProfile()));
}

static void
populatePaletteHelper(Local<Value> val, TermPalette &palette)
{
    if (val->IsObject()) {
        auto obj = Local<Object>::Cast(val);
        auto context = i->GetCurrentContext();

        for (unsigned k = 0; k < PALETTE_SIZE; ++k) {
            if (obj->Get(context, k).ToLocal(&val) && val->IsUint32()) {
                unsigned v = val->Uint32Value();
                palette.Termcolors::replace(k, v & PALETTE_VALUEMASK);
            }
        }
        if (obj->Get(context, v8name("dircolors")).ToLocal(&val) && val->IsString()) {
            palette.Dircolors::operator=(Dircolors(*String::Utf8Value(val)));
        }
    }
}

static Local<Object>
readPaletteHelper(const TermPalette &palette)
{
    auto context = i->GetCurrentContext();
    Local<Object> obj = Object::New(i);

    for (unsigned k = 0; k < PALETTE_SIZE; ++k) {
        obj->Set(context, k, Integer::NewFromUnsigned(i, palette[k])).ToChecked();
    }
    obj->Set(context, v8name("dircolors"), v8str(palette.dStr())).ToChecked();
    return obj;
}

static void
managerCreateTerminal(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(manager, TermManager);
    declare_arg_checked(server, ServerInstance, Server, args[0], h1);
    declare_arg_checked(profile, ProfileSettings, Profile, args[1], h2);
    declare_handle;

    if (!server->conn()) {
        v8throw(v8literal("Cannot create terminal: not connected"));
        return;
    }

    TermSig sig;
    sig.which = TermSig::Nothing;

    if (args[2]->IsObject()) {
        auto map = Local<Object>::Cast(args[2]);
        auto context = i->GetCurrentContext();
        Local<Value> val, key;

        if (map->Get(context, v8name("theme")).ToLocal(&val) && val->IsObject()) {
            declare_arg(theme, ThemeSettings, Theme, val, h);
            if (theme) {
                sig.which |= TermSig::Palette;
                sig.palette = theme->content();
            }
        }
        if (map->Get(context, v8name("palette")).ToLocal(&val) && val->IsObject()) {
            sig.which |= TermSig::Palette;
            populatePaletteHelper(val, sig.palette);
        }
        if (map->Get(context, v8name("layout")).ToLocal(&val) && !val->IsUndefined()) {
            sig.which |= TermSig::Layout;
            sig.layout = *String::Utf8Value(val);
        }
        if (map->Get(context, v8name("fills")).ToLocal(&val) && !val->IsUndefined()) {
            sig.which |= TermSig::Fills;
            sig.fills = *String::Utf8Value(val);
        }
        if (map->Get(context, v8name("badge")).ToLocal(&val) && !val->IsUndefined()) {
            sig.which |= TermSig::Badge;
            sig.badge = *String::Utf8Value(val);
        }
        if (map->Get(context, v8name("icon")).ToLocal(&val) && !val->IsUndefined()) {
            sig.which |= TermSig::Icon;
            sig.icon = *String::Utf8Value(val);
        }
        if (map->Get(context, v8name("attributes")).ToLocal(&val) && val->IsObject()) {
            auto obj = Local<Object>::Cast(val);
            auto keys = obj->GetOwnPropertyNames(context).ToLocalChecked();
            unsigned k = 0;
            while (keys->Get(context, k++).ToLocal(&key) && key->IsString()) {
                if (obj->Get(context, key).ToLocal(&val) && val->IsString())
                    sig.extraAttr[*String::Utf8Value(key)] = *String::Utf8Value(val);
            }
        }
        if (map->Get(context, v8name("environment")).ToLocal(&val) && val->IsObject()) {
            auto obj = Local<Object>::Cast(val);
            unsigned k = 0;
            while (obj->Get(context, k++).ToLocal(&val) && val->IsString()) {
                sig.extraEnv.append(*String::Utf8Value(val));
            }
        }
    }

    TermInstance *term = manager->createTerm(server, profile, &sig);
    v8ret(get_handle(Term, term));
}

static void
managerListPanes(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(manager, TermManager);
    declare_handle;

    const auto &stacks = manager->stacks();
    int n = stacks.size();
    Local<Array> arr = Array::New(i, n);
    auto context = i->GetCurrentContext();

    for (int k = 0; k < n; ++k) {
        auto *stack = stacks[k];
        auto index = Integer::New(i, stack->index());
        auto scrollport = get_handle(Scrollport, stack->currentScrollport());

        Local<Object> obj = Object::New(i);
        obj->Set(context, v8name("index"), index).ToChecked();
        obj->Set(context, v8name("viewport"), scrollport).ToChecked();
        arr->Set(context, k, obj).ToChecked();
    }
    v8ret(arr);
}

static void
managerListThemes(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_handle_checked;

    Local<Map> map = Map::New(i);
    auto context = i->GetCurrentContext();

    for (auto theme: g_settings->themes()) {
        auto h = get_handle(Theme, theme);
        map->Set(context, v8str(theme->name()), h).ToLocalChecked();
    }
    v8ret(map);
}

static void
managerPrompt(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(manager, TermManager);
    declare_handle;

    if (args[0]->IsObject()) {
        QString msg, initial;
        msg = L("%1: %2").arg(handle->base()->name(), *String::Utf8Value(args[1]));
        if (args[2]->IsString())
            initial = *String::Utf8Value(args[2]);

        auto *box = textBox(TR_TITLE1, msg, initial, manager->parentWidget());
        bool rc = g_listener->registerPluginPrompt(box);
        if (rc) {
            handle->startPrompt(Local<Object>::Cast(args[0]), box);
        }
        v8ret(Boolean::New(i, rc));
    } else {
        v8throw(v8literal("invalid argument"));
    }
}

static void
managerCopy(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(manager, TermManager);

    QString text = *String::Utf8Value(args[0]);
    QApplication::clipboard()->setText(text);
    manager->reportClipboardCopy(text.size());
}

static void
managerGetClientId(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    v8ret(v8str(g_listener->idStr()));
}

static void
managerGetClientAttribute(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    StringMap::const_iterator k;
    k = g_listener->attributes().find(*String::Utf8Value(args[0]));
    if (k != g_listener->attributes().cend())
        v8ret(v8str(k->second));
}

//
// IdBase Api
//
static void
idbaseGetAttribute(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(target, IdBase);
    auto k = target->attributes().constFind(*String::Utf8Value(args[0]));
    if (k != target->attributes().cend())
        v8ret(v8str(*k));
}

static void
idbaseGetActiveManager(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_handle_checked;
    v8ret(get_handle(Manager, g_listener->activeManager()));
}

static void
idbaseSetAttributeNotifier(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_handle_checked;

    QString str = *String::Utf8Value(args[1]);
    int timeout = args[2]->Int32Value();
    if (args[0]->IsObject() && args[1]->IsString() && !str.isEmpty()) {
        v8ret(handle->startAttrmon(Local<Object>::Cast(args[0]), str, timeout));
    } else {
        v8throw(v8literal("invalid argument"));
    }
}

//
// Server Api
//
static void
serverListServerTerminals(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(server, ServerInstance);
    declare_handle;

    const auto &terms = server->terms();
    int n = terms.size();
    Local<Array> arr = Array::New(i, n);
    auto context = i->GetCurrentContext();

    for (int k = 0; k < n; ++k) {
        arr->Set(context, k, get_handle(Term, terms[k])).ToChecked();
    }
    v8ret(arr);
}

static void
serverGetDefaultProfile(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(server, ServerInstance);
    declare_handle;

    QString profileName = server->serverInfo()->defaultProfile();
    v8ret(get_handle(Profile, g_settings->profile(profileName)));
}

//
// Term Api
//
static void
termGetProfile(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    declare_handle;
    v8ret(get_handle(Profile, term->profile()));
}

static void
termGetTheme(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    declare_handle;

    for (auto *theme: g_settings->themes())
        if (theme->content() == term->palette())
            v8ret(get_handle(Theme, theme));
}

static void
termGetServer(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    declare_handle;
    v8ret(get_handle(Server, term->server()));
}

static void
termGetPalette(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    declare_handle;
    v8ret(get_handle(Palette, term));
}

static void
termSetPalette(Local<String>, Local<Value> value, const PropertyCallbackInfo<void> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    TermPalette palette = term->palette();
    populatePaletteHelper(value, palette);
    term->setPalette(palette);
}

static void
termGetDircolors(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    v8ret(v8str(term->palette().dStr()));
}

static void
termSetDircolors(Local<String>, Local<Value> value, const PropertyCallbackInfo<void> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    TermPalette palette = term->palette();
    palette.Dircolors::operator=(Dircolors(*String::Utf8Value(value)));
    term->setPalette(palette);
}

static void
termGetColor(uint32_t index, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    if (index < PALETTE_SIZE) {
        unsigned v = term->palette().Termcolors::at(index);
        v8ret(Integer::NewFromUnsigned(i, v));
    }
}

static void
termSetColor(uint32_t index, Local<Value> value, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    if (index < PALETTE_SIZE) {
        TermPalette palette = term->palette();
        palette.Termcolors::replace(index, value->Uint32Value() & PALETTE_VALUEMASK);
        term->setPalette(palette);
    }
    v8ret(value);
}

static void
termGetFont(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    v8ret(v8str(term->font().toString()));
}

static void
termSetFont(Local<String>, Local<Value> value, const PropertyCallbackInfo<void> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    QFont font;
    if (font.fromString(*String::Utf8Value(value)))
        term->setFont(font);
}

static void
termGetBuffer(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    declare_handle;
    v8ret(get_handle(Buffer, term->buffers()));
}

static void
termCreateCursor(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    declare_handle;
    declare_priv(rowPriv);
    declare_priv(posPriv);
    declare_priv(pos2Priv);
    auto context = i->GetCurrentContext();
    auto cursor = Local<Object>::Cast(get_weak(Cursor, term->buffers()));
    int row = term->screen()->offset();
    if (args.Data()->BooleanValue())
        row += term->screen()->height();
    cursor->set_priv(rowPriv, Integer::New(i, row));
    auto zero = Integer::New(i, 0);
    cursor->set_priv(posPriv, zero);
    cursor->set_priv(pos2Priv, zero);
    v8ret(cursor);
}

static void
termNextRegionId(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    regionid_t result = term->buffers()->peekSemanticId();
    v8ret(Integer::NewFromUnsigned(i, result));
}

static LocalValue
populateRegionHelper(ApiHandle *handle, LocalContext context,
                     TermBuffers *buffers, const Region *region)
{
    declare_priv(rowPriv);
    declare_priv(posPriv);
    declare_priv(row2Priv);
    declare_priv(pos2Priv);
    declare_priv(idPriv);
    declare_priv(parentPriv);
    declare_priv(typePriv);
    declare_priv(flagsPriv);
    index_t origin = buffers->origin();
    auto reg = Local<Object>::Cast(get_weak(Region, buffers));
    reg->set_priv(rowPriv, Integer::NewFromUnsigned(i, region->startRow - origin));
    reg->set_priv(posPriv, Integer::NewFromUnsigned(i, region->startCol));
    reg->set_priv(row2Priv, Integer::NewFromUnsigned(i, region->endRow - origin));
    reg->set_priv(pos2Priv, Integer::NewFromUnsigned(i, region->endCol));
    reg->set_priv(idPriv, Integer::NewFromUnsigned(i, region->id()));
    reg->set_priv(parentPriv, Integer::NewFromUnsigned(i, region->parent));
    reg->set_priv(typePriv, Integer::New(i, region->type()));
    reg->set_priv(flagsPriv, Integer::NewFromUnsigned(i, region->flags));
    return reg;
}

static Region *
createRegionHelper(TermInstance *term, LocalValue spec,
                   size_t start, unsigned startPos, size_t end, unsigned endPos)
{
    TermBuffer *buffer = term->buffers()->bufferAt(start);
    size_t size = buffer->size();
    start -= buffer->offset();
    end -= buffer->offset();
    if (end >= size) {
        end = size;
        endPos = 0;
    }
    if (term->overlayActive() || start > end || (start == end && startPos >= endPos)) {
        return nullptr;
    }

    regionid_t id = term->buffers()->nextSemanticId();
    Region *r = new Region(Tsqt::RegionLink, buffer, id);
    r->flags = Tsq::HasStart|Tsq::HasEnd|Tsqt::Updating|Tsqt::Inline|Tsqt::Semantic;
    r->startRow = buffer->origin() + start;
    r->startCol = buffer->xByPos(r->startRow, startPos);
    r->endRow = buffer->origin() + end;
    r->endCol = buffer->xByPos(r->endRow, endPos);

    if (spec->IsObject()) {
        LocalString keys[SemanticParser::NKeys];
        SemanticParser::getSpecKeys(keys);
        SemanticParser::parseRegionSpec(Local<Object>::Cast(spec), r, keys);
    }

    buffer->insertSemanticRegion(r);
    return r;
}

static void
termCreateRegion(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    declare_arg_checked(b1, TermBuffers, Cursor, args[0], h1);
    declare_arg_checked(b2, TermBuffers, Cursor, args[1], h2);
    declare_priv(rowPriv);
    declare_priv(posPriv);
    auto context = i->GetCurrentContext();
    Region *r = createRegionHelper(term, args[2],
                                   h1->get_priv(rowPriv)->Uint32Value(),
                                   h1->get_priv(posPriv)->Uint32Value(),
                                   h2->get_priv(rowPriv)->Uint32Value(),
                                   h2->get_priv(posPriv)->Uint32Value());
    if (r) {
        declare_handle;
        v8ret(populateRegionHelper(handle, context, term->buffers(), r));
        term->buffers()->reportRegionChanged();
    }
}

static inline bool
createNoteHelper(TermInstance *term, LocalValue val, LocalContext context,
                 size_t start, unsigned startPos, size_t end, unsigned endPos)
{
    TermBuffer *buffer = term->buffers()->buffer0();
    size_t size = buffer->size();
    if (end >= size) {
        end = size;
        endPos = 0;
    }
    if (start > end || (start == end && startPos >= endPos)) {
        return false;
    }

    Region r(Tsqt::RegionUser, buffer, INVALID_REGION_ID);
    r.startRow = buffer->origin() + start;
    r.startCol = startPos;
    r.endRow = buffer->origin() + end;
    r.endCol = endPos;

    if (val->IsObject()) {
        auto map = Local<Object>::Cast(val);
        auto name = v8name(TSQ_ATTR_REGION_NOTECHAR);
        QString str;
        if (map->Get(context, name).ToLocal(&val) && val->IsString()) {
            str = *String::Utf8Value(val);
        } else {
            str = TermMark::base36(term->noteNum(), '+');
            term->incNoteNum();
        }
        r.attributes[g_attr_REGION_NOTECHAR] = str;

        name = v8name(TSQ_ATTR_REGION_NOTETEXT);
        if (map->Get(context, name).ToLocal(&val) && val->IsString()) {
            r.attributes[g_attr_REGION_NOTETEXT] = *String::Utf8Value(val);
        }
    }

    g_listener->pushRegionCreate(term, &r);
    return true;
}

static void
termCreateNote(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(term, TermInstance);
    declare_arg_checked(b1, TermBuffers, Cursor, args[0], h1);
    declare_arg_checked(b2, TermBuffers, Cursor, args[1], h2);
    declare_priv(rowPriv);
    declare_priv(posPriv);
    auto context = i->GetCurrentContext();
    bool rc = false;
    if (!term->overlayActive()) {
        rc = createNoteHelper(term, args[2], context,
                              h1->get_priv(rowPriv)->Uint32Value(),
                              h1->get_priv(posPriv)->Uint32Value(),
                              h2->get_priv(rowPriv)->Uint32Value(),
                              h2->get_priv(posPriv)->Uint32Value());
    }
    v8ret(Boolean::New(i, rc));
}

//
// Scrollport Api
//
static void
scrollGetTerm(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(scrollport, TermScrollport);
    declare_handle;
    v8ret(get_handle(Term, scrollport->term()));
}

static void
scrollGetBuffer(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(scrollport, TermScrollport);
    declare_handle;
    v8ret(get_handle(Buffer, scrollport->buffers()));
}

static void
scrollCreateCursor(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(scrollport, TermScrollport);
    declare_handle;
    declare_priv(rowPriv);
    declare_priv(posPriv);
    declare_priv(pos2Priv);
    auto context = i->GetCurrentContext();
    auto cursor = Local<Object>::Cast(get_weak(Cursor, scrollport->buffers()));
    int row = scrollport->offset();
    if (args.Data()->BooleanValue())
        row += scrollport->height();
    cursor->set_priv(rowPriv, Integer::New(i, row));
    auto zero = Integer::New(i, 0);
    cursor->set_priv(posPriv, zero);
    cursor->set_priv(pos2Priv, zero);
    v8ret(cursor);
}

static void
scrollCreateSelection(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(scrollport, TermScrollport);
    declare_arg_checked(b1, TermBuffers, Cursor, args[0], h1);
    declare_arg_checked(b2, TermBuffers, Cursor, args[1], h2);
    declare_handle;
    declare_priv(rowPriv);
    declare_priv(posPriv);
    auto context = i->GetCurrentContext();
    auto *sel = scrollport->buffers()->selection();
    sel->setAnchor(QPoint(h1->get_priv(posPriv)->Int32Value(),
                          h1->get_priv(rowPriv)->Int32Value()));
    sel->finish(QPoint(h2->get_priv(posPriv)->Int32Value(),
                       h2->get_priv(rowPriv)->Int32Value()));
    v8ret(populateRegionHelper(handle, context, scrollport->buffers(), sel));
}

static void
scrollCreateFlash(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(scrollport, TermScrollport);
    declare_arg_checked(b1, TermBuffers, Cursor, args[0], h1);
    declare_arg_checked(b2, TermBuffers, Cursor, args[1], h2);
    declare_handle;
    declare_priv(rowPriv);
    declare_priv(posPriv);
    auto context = i->GetCurrentContext();
    Local<Value> val = args[2];
    Local<Object> spec;
    int duration = 0;
    bool timer = false;

    if (val->IsObject()) {
        spec = Local<Object>::Cast(val);
        if (spec->Get(context, v8name("duration")).ToLocal(&val))
            duration = val->Int32Value();
        if (spec->Get(context, v8name("callback")).ToLocal(&val) && val->IsFunction())
            timer = true;
    }

    Region *r = createRegionHelper(scrollport->term(), args[2],
                                   h1->get_priv(rowPriv)->Uint32Value(),
                                   h1->get_priv(posPriv)->Uint32Value(),
                                   h2->get_priv(rowPriv)->Uint32Value(),
                                   h2->get_priv(posPriv)->Uint32Value());
    if (r) {
        TermBuffers *buffers = scrollport->buffers();
        v8ret(populateRegionHelper(handle, context, buffers, r));
        r->flash = new SemanticFlash(buffers, r->id());
        buffers->reportRegionChanged();
        r->flash->start(duration > 0 ? duration : 3);
        if (timer)
            handle->startTimer(spec, r->flash->timeout(), true);
    }
}

static void
scrollGetMousePosition(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(scrollport, TermScrollport);
    if (scrollport->focused()) {
        auto *arrange = static_cast<TermArrange*>(scrollport->parent());
        auto p = arrange->widget()->mousePosition();
        declare_handle;
        declare_priv(rowPriv);
        declare_priv(posPriv);
        declare_priv(pos2Priv);
        auto context = i->GetCurrentContext();
        TermBuffers *buffers = scrollport->buffers();
        auto cursor = Local<Object>::Cast(get_weak(Cursor, buffers));
        cursor->set_priv(rowPriv, Integer::New(i, p.y()));
        cursorPosHelper(cursor, context, buffers, p.y(), p.x(), posPriv, pos2Priv);
        v8ret(cursor);
    }
}

static void
scrollScrollTo(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(scrollport, TermScrollport);
    declare_arg_checked(b1, TermBuffers, Cursor, args[0], h1);
    declare_priv(rowPriv);
    auto context = i->GetCurrentContext();
    index_t row = scrollport->buffers()->origin() + h1->get_priv(rowPriv)->Uint32Value();
    scrollport->scrollToRow(row, args[1]->BooleanValue());
}

static void
scrollGetSelectedJob(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(scrollport, TermScrollport);
    auto context = i->GetCurrentContext();
    TermBuffers *buffers = scrollport->buffers();
    const auto *region = buffers->safeRegion(scrollport->activeJobId());
    if (region) {
        declare_handle;
        v8ret(populateRegionHelper(handle, context, buffers, region));
    }
}

static void
scrollGetSelection(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(scrollport, TermScrollport);
    TermBuffers *buffers = scrollport->buffers();
    if (buffers->selectionActive()) {
        declare_handle;
        auto context = i->GetCurrentContext();
        v8ret(populateRegionHelper(handle, context, buffers, buffers->selection()));
    }
}

//
// Buffer Api
//
static void
bufferGetLength(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    v8ret(Number::New(i, buffers->size()));
}

static void
bufferGetOrigin(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    v8ret(Number::New(i, buffers->origin()));
}

static void
bufferGetCursor(uint32_t index, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    if (index < buffers->size()) {
        declare_handle;
        declare_priv(rowPriv);
        declare_priv(posPriv);
        declare_priv(pos2Priv);
        auto context = i->GetCurrentContext();
        auto cursor = Local<Object>::Cast(get_weak(Cursor, buffers));
        // Place at start of given row
        cursor->set_priv(rowPriv, Integer::NewFromUnsigned(i, index));
        auto zero = Integer::New(i, 0);
        cursor->set_priv(posPriv, zero);
        cursor->set_priv(pos2Priv, zero);
        v8ret(cursor);
    }
}

//
// Cursor Api
//
static void
cursorGetProp(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    auto context = i->GetCurrentContext();
    Local<Value> rc;
    switch (args.Data()->Int32Value()) {
    case CursorRow: {
        declare_priv(rowPriv);
        rc = args.This()->get_priv(rowPriv);
        break;
    }
    case CursorOffset: {
        declare_priv(pos2Priv);
        rc = args.This()->get_priv(pos2Priv);
        break;
    }
    case CursorChar:
        declare_priv(posPriv);
        rc = args.This()->get_priv(posPriv);
        break;
    }
    v8ret(rc);
}

static void
cursorRowProp(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_priv(rowPriv);
    auto context = i->GetCurrentContext();
    size_t row = args.This()->get_priv(rowPriv)->Uint32Value();
    if (row < buffers->size()) {
        declare_priv(posPriv);
        const auto &crow = buffers->row(row);
        column_t pos = args.This()->get_priv(posPriv)->Uint32Value();
        Local<Value> rc;
        size_t ptr;

        switch (args.Data()->Int32Value()) {
        case CursorColumn:
            pos = buffers->xByPos(buffers->origin() + row, pos);
            rc = Integer::NewFromUnsigned(i, pos);
            break;
        case CursorColumns:
            pos = buffers->xSize(buffers->origin() + row);
            rc = Integer::NewFromUnsigned(i, pos);
            break;
        case CursorChars:
            rc = Integer::NewFromUnsigned(i, crow.size);
            break;
        case CursorContinuation:
            rc = Boolean::New(i, crow.flags & Tsq::Continuation);
            break;
        case CursorFlags:
            rc = Integer::NewFromUnsigned(i, crow.flags);
            break;
        case CursorText:
            rc = v8str(crow.str);
            break;
        case CursorTextBefore:
            ptr = ptrByPos(buffers, crow.str, pos);
            rc = v8str(crow.str.substr(0, ptr));
            break;
        case CursorTextAfter:
            ptr = ptrByPos(buffers, crow.str, pos);
            rc = v8str(crow.str.substr(ptr));
            break;
        }
        v8ret(rc);
    }
}

static void
cursorCompareTo(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_arg_checked(b2, TermBuffers, Cursor, args[0], h2);
    declare_priv(rowPriv);
    declare_priv(posPriv);
    auto context = i->GetCurrentContext();
    auto pos1 = std::make_pair(args.This()->get_priv(rowPriv)->Int32Value(),
                               args.This()->get_priv(posPriv)->Int32Value());
    auto pos2 = std::make_pair(h2->get_priv(rowPriv)->Int32Value(),
                               h2->get_priv(posPriv)->Int32Value());
    v8ret(Integer::New(i, (pos1 > pos2) - (pos1 < pos2)));
}

static void
cursorClone(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_handle;
    declare_priv(rowPriv);
    declare_priv(posPriv);
    declare_priv(pos2Priv);
    auto context = i->GetCurrentContext();
    auto cursor = Local<Object>::Cast(get_weak(Cursor, buffers));
    cursor->set_priv(rowPriv, args.This()->get_priv(rowPriv));
    cursor->set_priv(posPriv, args.This()->get_priv(posPriv));
    cursor->set_priv(pos2Priv, args.This()->get_priv(pos2Priv));
    v8ret(cursor);
}

static void
cursorMeasureColumns(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    if (args[0]->IsString()) {
        unsigned len = xSize(buffers, *String::Utf8Value(args[0]));
        v8ret(Integer::NewFromUnsigned(i, len));
    } else {
        v8throw(v8literal("invalid argument"));
    }
}

static void
cursorMeasureChars(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    if (args[0]->IsString()) {
        unsigned len = posSize(buffers, *String::Utf8Value(args[0]));
        v8ret(Integer::NewFromUnsigned(i, len));
    } else {
        v8throw(v8literal("invalid argument"));
    }
}

static void
cursorMove(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_priv(rowPriv);
    declare_priv(posPriv);
    declare_priv(pos2Priv);
    auto context = i->GetCurrentContext();
    size_t size, row = args.This()->get_priv(rowPriv)->Uint32Value();
    column_t max, pos = args.This()->get_priv(posPriv)->Int32Value();
    int offset;
    const Region *region;
    Local<Value> rc = args.This();

    switch(args.Data()->Int32Value()) {
    case ByRows:
        row += args[0]->Int32Value();
        break;
    case ToRow:
        row = args[0]->Uint32Value();
        break;
    case ByOffset:
        offset = args.This()->get_priv(pos2Priv)->Int32Value();
        offset += args[0]->Int32Value();
        if (offset < 0)
            offset = 0;
        pos = posByJavascript(buffers, row, offset);
        goto validateSkip;
    case ToOffset:
        pos = posByJavascript(buffers, row, args[0]->Uint32Value());
        goto validateSkip;
    case ByColumns:
        offset = buffers->xByPos(row, pos) + args[0]->Int32Value();
        if (offset < 0)
            offset = 0;
        pos = buffers->posByX(row, pos);
        goto validateSkip;
    case ToColumn:
        pos = buffers->posByX(row, args[0]->Int32Value());
        goto validateSkip;
    case ByChars:
        pos += args[0]->Int32Value();
        goto validatePos;
    case ToChar:
        pos = args[0]->Uint32Value();
        goto validatePos;
    case ToEnd:
        pos = INVALID_COLUMN;
        goto validatePos;
    case ToNextJob:
        region = buffers->findRegionByRow(Tsqt::RegionJob, buffers->origin() + row);
        region = buffers->nextRegion(Tsqt::RegionJob, region ? region->id() : INVALID_REGION_ID);
        goto checkRegion;
    case ToPrevJob:
        region = buffers->findRegionByRow(Tsqt::RegionJob, buffers->origin() + row);
        region = buffers->prevRegion(Tsqt::RegionJob, region ? region->id() : INVALID_REGION_ID);
        goto checkRegion;
    case ToNextNote:
        region = buffers->findRegionByRow(Tsqt::RegionUser, buffers->origin() + row);
        region = buffers->nextRegion(Tsqt::RegionUser, region ? region->id() : INVALID_REGION_ID);
        goto checkRegion;
    case ToPrevNote:
        region = buffers->findRegionByRow(Tsqt::RegionUser, buffers->origin() + row);
        region = buffers->prevRegion(Tsqt::RegionUser, region ? region->id() : INVALID_REGION_ID);
        checkRegion:
        if (region) {
            declare_handle;
            rc = populateRegionHelper(handle, context, buffers, region);
            row = region->startRow - buffers->origin();
            pos = region->startCol;
            break;
        } else {
            // bailout
            return;
        }
    case ToFirstRow:
        row = 0;
        pos = 0;
        break;
    case ToLastRow:
        row = buffers->size() - 1;
        pos = 0;
        break;
    case ToScreenStart:
        row = buffers->term()->screen()->offset();
        pos = 0;
        break;
    case ToScreenEnd:
        row = buffers->size();
        pos = 0;
        break;
    case ToScreenCursor:
        QPoint p = buffers->term()->screen()->cursor();
        row = p.y();
        pos = p.x();
        break;
    }

    // Validate new position
    size = buffers->size();
    if (row > size)
        row = size;
    args.This()->set_priv(rowPriv, Integer::NewFromUnsigned(i, row));
validatePos:
    max = buffers->safeRow(row).size;
    if (pos > max)
        pos = max;
validateSkip:
    cursorPosHelper(args.This(), context, buffers, row, pos, posPriv, pos2Priv);
    v8ret(rc);
}

static void
cursorGetJob(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_priv(rowPriv);
    auto context = i->GetCurrentContext();
    index_t row = buffers->origin() + args.This()->get_priv(rowPriv)->Uint32Value();
    const auto *region = buffers->findRegionByRow(Tsqt::RegionJob, row);
    if (region) {
        declare_handle;
        v8ret(populateRegionHelper(handle, context, buffers, region));
    }
}

static void
cursorToPersistent(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_handle;
    declare_priv(rowPriv);
    declare_priv(row2Priv);
    declare_priv(posPriv);
    auto context = i->GetCurrentContext();
    auto persistent = Local<Object>::Cast(get_weak(Persistent, buffers));
    index_t row = buffers->offset() + args.This()->get_priv(rowPriv)->Uint32Value();
    uint32_t rowhi = row >> 32;
    uint32_t rowlo = row & 0xffffffff;
    persistent->set_priv(rowPriv, Integer::NewFromUnsigned(i, rowlo));
    persistent->set_priv(row2Priv, Integer::NewFromUnsigned(i, rowhi));
    persistent->set_priv(posPriv, args.This()->get_priv(posPriv));
    v8ret(persistent);
}

static void
persistentToCursor(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_priv(rowPriv);
    declare_priv(row2Priv);
    declare_priv(posPriv);
    auto context = i->GetCurrentContext();
    index_t row = args.This()->get_priv(row2Priv)->Uint32Value();
    row = (row << 32) | args.This()->get_priv(rowPriv)->Uint32Value();
    row -= buffers->origin();
    if (row < buffers->size()) {
        declare_handle;
        auto cursor = Local<Object>::Cast(get_weak(Cursor, buffers));
        cursor->set_priv(rowPriv, Integer::NewFromUnsigned(i, row));
        column_t pos = args.This()->get_priv(posPriv)->Uint32Value();
        declare_priv(pos2Priv);
        cursorPosHelper(cursor, context, buffers, row, pos, posPriv, pos2Priv);
        v8ret(cursor);
    }
}

//
// Region Api
//
static void
regionGetProp(Local<String>, const PropertyCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    LocalPrivate priv;
    if (args.Data()->Int32Value() == RegionId) {
        declare_priv(idPriv);
        priv = idPriv;
    } else {
        declare_priv(flagsPriv);
        priv = flagsPriv;
    }
    auto context = i->GetCurrentContext();
    v8ret(args.This()->get_priv(priv));
}

static void
regionGetAttribute(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_priv(idPriv);
    declare_priv(typePriv);
    auto context = i->GetCurrentContext();
    regionid_t id = args.This()->get_priv(idPriv)->Uint32Value();
    const Region *region;
    switch (args.This()->get_priv(typePriv)->Int32Value()) {
    case Tsqt::RegionLink:
    case Tsqt::RegionSemantic:
        region = buffers->safeSemantic(id);
        break;
    default:
        region = buffers->safeRegion(id);
        break;
    }
    if (region && args[0]->IsString()) {
        auto k = region->attributes.constFind(*String::Utf8Value(args[0]));
        if (k != region->attributes.cend()) {
            v8ret(v8str(*k));
        }
    }
}

static void
regionGetJob(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_priv(parentPriv);
    declare_priv(typePriv);
    auto context = i->GetCurrentContext();

    switch (args.This()->get_priv(typePriv)->Int32Value()) {
    case Tsqt::RegionPrompt:
    case Tsqt::RegionCommand:
    case Tsqt::RegionOutput:
        break;
    case Tsqt::RegionJob:
        v8ret(args.This());
        // fallthru
    default:
        return;
    }

    regionid_t id = args.This()->get_priv(parentPriv)->Uint32Value();
    const auto *region = buffers->safeRegion(id);
    if (region) {
        declare_handle;
        v8ret(populateRegionHelper(handle, context, buffers, region));
    }
}

static void
regionGetChild(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_priv(idPriv);
    declare_priv(typePriv);
    auto context = i->GetCurrentContext();

    if (args.This()->get_priv(typePriv)->Int32Value() == Tsqt::RegionJob)
    {
        regionid_t id = args.This()->get_priv(idPriv)->Uint32Value();
        auto type = (Tsqt::RegionType)args.Data()->Int32Value();
        const auto *region = buffers->findRegionByParent(type, id);
        if (region) {
            declare_handle;
            v8ret(populateRegionHelper(handle, context, buffers, region));
        }
    }
}

static void
regionCreateCursor(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(buffers, TermBuffers);
    declare_handle;
    declare_priv(rowPriv);
    declare_priv(posPriv);
    declare_priv(pos2Priv);
    auto context = i->GetCurrentContext();
    auto cursor = Local<Object>::Cast(get_weak(Cursor, buffers));
    Local<Private> rpriv, ppriv;
    if (args.Data()->BooleanValue()) {
        declare_priv(row2Priv);
        rpriv = row2Priv;
        ppriv = pos2Priv;
    } else {
        rpriv = rowPriv;
        ppriv = posPriv;
    }
    Local<Value> val;
    cursor->set_priv(rowPriv, val = args.This()->get_priv(rpriv));
    size_t row = val->Uint32Value();
    column_t pos = args.This()->get_priv(ppriv)->Uint32Value();
    cursorPosHelper(cursor, context, buffers, row, pos, posPriv, pos2Priv);
    v8ret(cursor);
}

//
// Settings Api
//
static LocalValue
makeStringListHelper(const QStringList &list)
{
    int n = list.size();
    Local<Array> arr = Array::New(i, n);
    auto context = i->GetCurrentContext();

    for (int k = 0; k < n; ++k) {
        arr->Set(context, k, v8str(list[k])).ToChecked();
    }
    return arr;
}

static LocalValue
makeSizeHelper(const QSize &size)
{
    Local<Array> arr = Array::New(i, 2);
    auto context = i->GetCurrentContext();

    arr->Set(context, 0, Integer::New(i, size.width())).ToChecked();
    arr->Set(context, 1, Integer::New(i, size.height())).ToChecked();
    return arr;
}

static void
settingsGetSetting(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(base, SettingsBase);
    const SettingDef *def = base->getDef(*String::Utf8Value(args[0]));
    if (def && def->property) {
        QVariant value = base->property(def->property);
        Local<Value> result;
        switch (def->type) {
        case QVariant::Bool:
            result = Boolean::New(i, value.toBool());
            break;
        case QVariant::Int:
            result = Integer::New(i, value.toInt());
            break;
        case QVariant::UInt:
            result = Integer::NewFromUnsigned(i, value.toUInt());
            break;
        case QVariant::StringList:
            result = makeStringListHelper(value.toStringList());
            break;
        case QVariant::Size:
            result = makeSizeHelper(value.toSize());
            break;
        default:
            result = v8str(value.toString());
        }
        v8ret(result);
    }
}

static void
profileGetKeymap(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(profile, ProfileSettings);
    declare_handle;
    v8ret(get_handle(Keymap, profile->keymap()));
}

static void
profileGetPalette(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(profile, ProfileSettings);
    v8ret(readPaletteHelper(profile->content()));
}

static void
themeGetPalette(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(theme, ThemeSettings);
    v8ret(readPaletteHelper(theme->content()));
}

static void
keymapLookupShortcut(const FunctionCallbackInfo<Value> &args)
{
    HandleScope scope(i);
    declare_target_checked(keymap, TermKeymap);
    TermShortcut result;
    if (keymap->lookupShortcut(*String::Utf8Value(args[0]), 0, &result)) {
        auto context = i->GetCurrentContext();
        Local<Object> obj = Object::New(i);
        obj->Set(context, v8name("expression"), v8str(result.expression)).ToChecked();
        obj->Set(context, v8name("additional"), v8str(result.additional)).ToChecked();
        v8ret(obj);
    }
}

//
// Module
//
static void
cbCancelMonitor(const FunctionCallbackInfo<Value> &args)
{
    declare_handle_checked;
    declare_target(monitor, ApiMonitor);
    monitor->canceling = true;
    monitor->finalize();
    emit handle->requestCancel(monitor);
}

static void
setupTemplate(Local<ObjectTemplate> &tmpl, bool primary)
{
    tmpl = ObjectTemplate::New(i);
    tmpl->SetInternalFieldCount(ApiHandle::NFields);
    // Add common fields
    tmpl->v8method("isValid", cbIsValid);
    if (primary) {
        tmpl->v8method("getPrivateData", cbGetPrivateData);
        tmpl->v8method("setPrivateData", cbSetPrivateData);
        tmpl->v8method("setInterval", cbSetInterval);
    }
}

void
ActionFeature::initializeApi(PersistentObjectTemplate *apitmpl)
{
    s_rowPriv.Reset(i, Private::New(i));
    s_posPriv.Reset(i, Private::New(i));
    s_row2Priv.Reset(i, Private::New(i));
    s_pos2Priv.Reset(i, Private::New(i));
    s_idPriv.Reset(i, Private::New(i));
    s_parentPriv.Reset(i, Private::New(i));
    s_typePriv.Reset(i, Private::New(i));
    s_flagsPriv.Reset(i, Private::New(i));
    Local<ObjectTemplate> tmpl;

    // Manager
    setupTemplate(tmpl, true);
    tmpl->v8method("invoke", managerInvoke);
    tmpl->v8method("notifySend", managerNotifySend);
    tmpl->v8method("listServers", managerListServers);
    tmpl->v8method("getServerById", managerGetServerById);
    tmpl->v8method("getActiveServer", managerGetActiveServer);
    tmpl->v8method("getLocalServer", managerGetLocalServer);
    tmpl->v8method("listTerminals", managerListTerminals);
    tmpl->v8method("getTerminalById", managerGetTerminalById);
    tmpl->v8method("getActiveTerminal", managerGetActiveTerminal);
    tmpl->v8method("createTerminal", managerCreateTerminal);
    tmpl->v8method("getActiveViewport", managerGetActiveScrollport);
    tmpl->v8method("getGlobalSettings", managerGetGlobalSettings);
    tmpl->v8method("getDefaultProfile", managerGetDefaultProfile);
    tmpl->v8method("listThemes", managerListThemes);
    tmpl->v8method("listPanes", managerListPanes);
    tmpl->v8method("prompt", managerPrompt);
    tmpl->v8method("copy", managerCopy);
    tmpl->SetAccessor(v8name("clientId"), managerGetClientId);
    tmpl->v8method("getClientAttribute", managerGetClientAttribute);
    apitmpl[ApiHandle::Manager].Reset(i, tmpl);

    // Server
    setupTemplate(tmpl, true);
    tmpl->v8switchget("id", cbGetProp, QVariant::String);
    tmpl->v8method("getAttribute", idbaseGetAttribute);
    tmpl->v8method("getActiveManager", idbaseGetActiveManager);
    tmpl->v8method("listTerminals", serverListServerTerminals);
    tmpl->v8method("getDefaultProfile", serverGetDefaultProfile);
    tmpl->v8method("setAttributeNotifier", idbaseSetAttributeNotifier);
    apitmpl[ApiHandle::Server].Reset(i, tmpl);

    // Term
    setupTemplate(tmpl, true);
    tmpl->v8switchget("id", cbGetProp, QVariant::String);
    tmpl->SetAccessor(v8name("palette"), termGetPalette, termSetPalette);
    tmpl->SetAccessor(v8name("font"), termGetFont, termSetFont);
    tmpl->v8switchset("layout", cbGetProp, cbSetProp, QVariant::String);
    tmpl->v8switchset("fills", cbGetProp, cbSetProp, QVariant::String);
    tmpl->v8switchset("badge", cbGetProp, cbSetProp, QVariant::String);
    tmpl->v8switchset("icon", cbGetProp, cbSetProp, QVariant::String);
    tmpl->SetAccessor(v8name("rows"), termGetBuffer);
    tmpl->v8switchget("height", cbGetProp, QVariant::Int);
    tmpl->v8switchget("width", cbGetProp, QVariant::Int);
    tmpl->v8switchget("ours", cbGetProp, QVariant::Bool);
    tmpl->v8method("getAttribute", idbaseGetAttribute);
    tmpl->v8method("getActiveManager", idbaseGetActiveManager);
    tmpl->v8method("getProfile", termGetProfile);
    tmpl->v8method("getTheme", termGetTheme);
    tmpl->v8method("getServer", termGetServer);
    tmpl->v8method("setAttributeNotifier", idbaseSetAttributeNotifier);
    tmpl->v8switch("getStart", termCreateCursor, 0);
    tmpl->v8switch("getEnd", termCreateCursor, 1);
    tmpl->v8method("nextRegionId", termNextRegionId);
    tmpl->v8method("createRegion", termCreateRegion);
    tmpl->v8method("createNote", termCreateNote);
    apitmpl[ApiHandle::Term].Reset(i, tmpl);

    // Scrollport
    setupTemplate(tmpl, true);
    tmpl->v8switchget("height", cbGetProp, QVariant::Int);
    tmpl->v8switchget("width", cbGetProp, QVariant::Int);
    tmpl->v8switchget("primary", cbGetProp, QVariant::Bool);
    tmpl->v8switchget("active", cbGetProp, QVariant::Bool);
    tmpl->SetAccessor(v8name("rows"), scrollGetBuffer);
    tmpl->v8method("getTerminal", scrollGetTerm);
    tmpl->v8method("getSelectedJob", scrollGetSelectedJob);
    tmpl->v8switch("getStart", scrollCreateCursor, 0);
    tmpl->v8switch("getEnd", scrollCreateCursor, 1);
    tmpl->v8method("getSelection", scrollGetSelection);
    tmpl->v8method("createSelection", scrollCreateSelection);
    tmpl->v8method("createFlash", scrollCreateFlash);
    tmpl->v8method("getMousePosition", scrollGetMousePosition);
    tmpl->v8method("scrollTo", scrollScrollTo);
    apitmpl[ApiHandle::Scrollport].Reset(i, tmpl);

    // Global
    setupTemplate(tmpl, true);
    tmpl->v8method("getSetting", settingsGetSetting);
    apitmpl[ApiHandle::Global].Reset(i, tmpl);

    // Profile
    setupTemplate(tmpl, true);
    tmpl->v8switchget("name", cbGetProp, QVariant::String);
    tmpl->v8method("getSetting", settingsGetSetting);
    tmpl->v8method("getKeymap", profileGetKeymap);
    tmpl->v8method("getPalette", profileGetPalette);
    apitmpl[ApiHandle::Profile].Reset(i, tmpl);

    // Theme
    setupTemplate(tmpl, true);
    tmpl->v8switchget("name", cbGetProp, QVariant::String);
    tmpl->v8method("getSetting", settingsGetSetting);
    tmpl->v8method("getPalette", themeGetPalette);
    apitmpl[ApiHandle::Theme].Reset(i, tmpl);

    // Keymap
    setupTemplate(tmpl, true);
    tmpl->v8switchget("name", cbGetProp, QVariant::String);
    tmpl->v8method("lookupShortcut", keymapLookupShortcut);
    apitmpl[ApiHandle::Keymap].Reset(i, tmpl);

    // Palette (Term)
    setupTemplate(tmpl, false);
    tmpl->SetAccessor(v8name("dircolors"), termGetDircolors, termSetDircolors);
    tmpl->SetHandler(IndexedPropertyHandlerConfiguration(termGetColor, termSetColor));
    apitmpl[ApiHandle::Palette].Reset(i, tmpl);

    // Buffer (Buffers)
    setupTemplate(tmpl, false);
    tmpl->SetAccessor(v8name("length"), bufferGetLength);
    tmpl->SetAccessor(v8name("origin"), bufferGetOrigin);
    tmpl->SetHandler(IndexedPropertyHandlerConfiguration(bufferGetCursor));
    apitmpl[ApiHandle::Buffer].Reset(i, tmpl);

    // Cursor (Buffers weak)
    setupTemplate(tmpl, false);
    tmpl->v8switchget("row", cursorGetProp, CursorRow);
    tmpl->v8switchget("offset", cursorGetProp, CursorOffset);
    tmpl->v8switchget("column", cursorGetProp, CursorColumn);
    tmpl->v8switchget("character", cursorRowProp, CursorChar);
    tmpl->v8switchget("text", cursorRowProp, CursorText);
    tmpl->v8switchget("textBefore", cursorRowProp, CursorTextBefore);
    tmpl->v8switchget("textAfter", cursorRowProp, CursorTextAfter);
    tmpl->v8switchget("columns", cursorRowProp, CursorColumns);
    tmpl->v8switchget("characters", cursorRowProp, CursorChars);
    tmpl->v8switchget("continuation", cursorRowProp, CursorContinuation);
    tmpl->v8switchget("flags", cursorRowProp, CursorFlags);
    tmpl->v8method("compareTo", cursorCompareTo);
    tmpl->v8method("clone", cursorClone);
    tmpl->v8method("toPersistent", cursorToPersistent);
    tmpl->v8method("getJob", cursorGetJob);
    tmpl->v8method("measureColumns", cursorMeasureColumns);
    tmpl->v8method("measureCharacters", cursorMeasureChars);
    tmpl->v8switch("moveByRows", cursorMove, ByRows);
    tmpl->v8switch("moveToRow", cursorMove, ToRow);
    tmpl->v8switch("moveByOffset", cursorMove, ByOffset);
    tmpl->v8switch("moveToOffset", cursorMove, ToOffset);
    tmpl->v8switch("moveByColumns", cursorMove, ByColumns);
    tmpl->v8switch("moveToColumn", cursorMove, ToColumn);
    tmpl->v8switch("moveByCharacters", cursorMove, ByChars);
    tmpl->v8switch("moveToCharacter", cursorMove, ToChar);
    tmpl->v8switch("moveToEnd", cursorMove, ToEnd);
    tmpl->v8switch("moveToNextJob", cursorMove, ToNextJob);
    tmpl->v8switch("moveToPreviousJob", cursorMove, ToPrevJob);
    tmpl->v8switch("moveToNextNote", cursorMove, ToNextNote);
    tmpl->v8switch("moveToPreviousNote", cursorMove, ToPrevNote);
    tmpl->v8switch("moveToFirstRow", cursorMove, ToFirstRow);
    tmpl->v8switch("moveToLastRow", cursorMove, ToLastRow);
    tmpl->v8switch("moveToScreenStart", cursorMove, ToScreenStart);
    tmpl->v8switch("moveToScreenEnd", cursorMove, ToScreenEnd);
    tmpl->v8switch("moveToScreenCursor", cursorMove, ToScreenCursor);
    apitmpl[ApiHandle::Cursor].Reset(i, tmpl);

    // Persistent (Buffers weak)
    setupTemplate(tmpl, false);
    tmpl->v8method("toCursor", persistentToCursor);
    apitmpl[ApiHandle::Persistent].Reset(i, tmpl);

    // Region (Buffers weak)
    setupTemplate(tmpl, false);
    tmpl->v8switchget("id", regionGetProp, RegionId);
    tmpl->v8switchget("flags", regionGetProp, RegionFlags);
    tmpl->v8method("getAttribute", regionGetAttribute);
    tmpl->v8method("getJob", regionGetJob);
    tmpl->v8switch("getPrompt", regionGetChild, Tsqt::RegionPrompt);
    tmpl->v8switch("getCommand", regionGetChild, Tsqt::RegionCommand);
    tmpl->v8switch("getOutput", regionGetChild, Tsqt::RegionOutput);
    tmpl->v8switch("getStart", regionCreateCursor, 0);
    tmpl->v8switch("getEnd", regionCreateCursor, 1);
    apitmpl[ApiHandle::Region].Reset(i, tmpl);

    // Monitor
    tmpl = ObjectTemplate::New(i);
    tmpl->SetInternalFieldCount(ApiHandle::NFields);
    tmpl->v8method("cancel", cbCancelMonitor);
    apitmpl[ApiHandle::Monitor].Reset(i, tmpl);
}

void
ActionFeature::teardownApi(PersistentObjectTemplate *apitmpl)
{
    for (int i = 0; i < ApiHandle::NTypes; ++i)
        apitmpl[i].Reset();

    s_rowPriv.Reset();
    s_posPriv.Reset();
    s_row2Priv.Reset();
    s_pos2Priv.Reset();
    s_idPriv.Reset();
    s_parentPriv.Reset();
    s_typePriv.Reset();
    s_flagsPriv.Reset();
}
