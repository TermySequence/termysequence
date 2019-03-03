// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/config.h"
#include "app/flags.h"
#include "app/plugin.h"
#include "app/watchdog.h"
#include "sembase.h"
#include "region.h"
#include "buffer.h"
#include "buffers.h"
#include "term.h"
#include "server.h"
#include "settings/settings.h"
#include "lib/unicode.h"
#include "lib/utf8.h"

using namespace v8;

#define declare_job \
    const auto *job = \
    static_cast<const Region*>(args.This()->GetAlignedPointerFromInternalField(0));
#define declare_thiz \
    auto *thiz = \
    static_cast<SemanticParser*>(args.This()->GetAlignedPointerFromInternalField(0));

//
// Callbacks
//
static void
cbNextRegionId(const FunctionCallbackInfo<Value> &args)
{
    declare_thiz;
    v8ret(Integer::NewFromUnsigned(i, thiz->peekRegionId()));
}

static void
cbCreateRegion(const FunctionCallbackInfo<Value> &args)
{
    declare_thiz;
    regionid_t id = INVALID_REGION_ID;

    if (thiz->parsing()) {
        id = thiz->createRegion(args[2], args[0]->Uint32Value(),
                                args[1]->Uint32Value());
    }
    v8ret(Integer::NewFromUnsigned(i, id));
}

static void
cbCreateRegionAt(const FunctionCallbackInfo<Value> &args)
{
    declare_thiz;
    regionid_t id = INVALID_REGION_ID;

    if (thiz->parsing()) {
        id = thiz->createRegion(args[4], args[0]->Uint32Value(),
                                args[1]->Uint32Value(),
                                args[2]->Uint32Value(),
                                args[3]->Uint32Value());
    }
    v8ret(Integer::NewFromUnsigned(i, id));
}

static void
cbJobAttribute(const FunctionCallbackInfo<Value> &args)
{
    declare_job;
    auto k = job->attributes.constFind(*String::Utf8Value(args[0]));
    if (k != job->attributes.cend())
        v8ret(v8str(*k));
}

static void
cbTerminalAttribute(const FunctionCallbackInfo<Value> &args)
{
    declare_job;
    const auto &attributes = job->buffer()->term()->attributes();
    auto k = attributes.constFind(*String::Utf8Value(args[0]));
    if (k != attributes.cend())
        v8ret(v8str(*k));
}

static void
cbServerAttribute(const FunctionCallbackInfo<Value> &args)
{
    declare_job;
    const auto &attributes = job->buffer()->term()->server()->attributes();
    auto k = attributes.constFind(*String::Utf8Value(args[0]));
    if (k != attributes.cend())
        v8ret(v8str(*k));
}

static void
cbJobId(const FunctionCallbackInfo<Value> &args)
{
    declare_job;
    v8ret(Integer::NewFromUnsigned(i, job->id()));
}

static void
cbTerminalId(const FunctionCallbackInfo<Value> &args)
{
    declare_job;
    v8ret(v8str(job->buffer()->term()->idStr()));
}

static void
cbServerId(const FunctionCallbackInfo<Value> &args)
{
    declare_job;
    v8ret(v8str(job->buffer()->term()->server()->idStr()));
}

//
// Feature
//
static PersistentObjectTemplate s_matchtmpl;
static PersistentObjectTemplate s_processtmpl;
static PersistentString s_startkey;
static PersistentString s_processkey;
static PersistentString s_finishkey;
static PersistentString s_iconkey;
static PersistentString s_tooltipkey;
static PersistentString s_action1key;
static PersistentString s_menukey;
static PersistentString s_dragkey;
static PersistentString s_urikey;

SemanticFeature::SemanticFeature(Plugin *parent, const QString &name,
                                 LocalFunction matcher, ParserType type) :
    Feature(SemanticFeatureType, parent, name),
    m_type(type)
{
    m_matcher.Reset(i, matcher);
}

inline SemanticParser *
SemanticFeature::getParser(TermBuffer *buffer, Region *output, LocalContext context,
                           LocalObject theirs)
{
    switch (m_type) {
    case FastSemantic:
        return new FastSemanticParser(buffer, output, context, theirs);
    default:
        return new SemanticParser(buffer, output, context, theirs);
    }
}

SemanticParser *
SemanticFeature::createParser(TermBuffer *buffer, const Region *job, Region *output)
{
    if ((job->flags & (Tsq::HasCommand|Tsq::EmptyCommand)) != Tsq::HasCommand ||
        output->flags & Tsq::Overwritten || !i)
        return nullptr;

    QString commstr = job->attributes.value(g_attr_REGION_COMMAND).trimmed();
    if (commstr.isEmpty())
        return nullptr;

    g_watchdog->begin();

    HandleScope outer(i);
    auto comm = v8str(commstr);
    auto dir = v8str(job->attributes.value(g_attr_REGION_PATH));
    auto ourstmpl = Local<ObjectTemplate>::New(i, s_matchtmpl);

    for (auto plugin: g_settings->plugins()) {
        HandleScope inner(i);
        auto context = plugin->context();
        Context::Scope context_scope(context);
        TryCatch trycatch(i);

        auto ours = ourstmpl->NewInstance();
        ours->SetAlignedPointerInInternalField(0, const_cast<Region*>(job));

        const int argc = 3;
        Local<Value> argv[argc] = { ours, comm, dir }, result;

        for (auto base: plugin->semantics()) {
            auto func = Local<Function>::New(i, base->m_matcher);
            g_watchdog->enter();
            bool rc = func->Call(context, context->Global(), argc, argv).ToLocal(&result);
            g_watchdog->leave();
            if (rc) {
                if (result->IsObject())
                {
                    auto theirs = Local<Object>::Cast(result);
                    auto *p = base->getParser(buffer, output, context, theirs);
                    p->setPluginName(plugin->name());
                    p->setParserName(base->name());
                    return p;
                }
            }
            else if (!Plugin::recover(plugin->name(), base->name(), "match", trycatch)) {
                return nullptr;
            }
        }
    }

    return nullptr;
}

void
SemanticFeature::initialize()
{
    auto matchtmpl = ObjectTemplate::New(i);
    matchtmpl->SetInternalFieldCount(1);
    matchtmpl->v8method("getJobAttribute", cbJobAttribute);
    matchtmpl->v8method("getJobId", cbJobId);
    matchtmpl->v8method("getTerminalAttribute", cbTerminalAttribute);
    matchtmpl->v8method("getTerminalId", cbTerminalId);
    matchtmpl->v8method("getServerAttribute", cbServerAttribute);
    matchtmpl->v8method("getServerId", cbServerId);
    s_matchtmpl.Reset(i, matchtmpl);

    auto processtmpl = ObjectTemplate::New(i);
    processtmpl->SetInternalFieldCount(1);
    processtmpl->v8method("createRegion", cbCreateRegion);
    processtmpl->v8method("createRegionAt", cbCreateRegionAt);
    processtmpl->v8method("nextRegionId", cbNextRegionId);
    s_processtmpl.Reset(i, processtmpl);

    s_startkey.Reset(i, v8name("start"));
    s_processkey.Reset(i, v8name("process"));
    s_finishkey.Reset(i, v8name("finish"));
    s_iconkey.Reset(i, v8name(TSQ_ATTR_CONTENT_ICON));
    s_tooltipkey.Reset(i, v8name(TSQ_ATTR_CONTENT_TOOLTIP));
    s_action1key.Reset(i, v8name(TSQ_ATTR_CONTENT_ACTION1));
    s_menukey.Reset(i, v8name(TSQ_ATTR_CONTENT_MENU));
    s_dragkey.Reset(i, v8name(TSQ_ATTR_CONTENT_DRAG));
    s_urikey.Reset(i, v8name(TSQ_ATTR_CONTENT_URI));
}

void
SemanticFeature::teardown()
{
    s_urikey.Reset();
    s_dragkey.Reset();
    s_menukey.Reset();
    s_action1key.Reset();
    s_tooltipkey.Reset();
    s_iconkey.Reset();
    s_finishkey.Reset();
    s_processkey.Reset();
    s_startkey.Reset();

    s_processtmpl.Reset();
    s_matchtmpl.Reset();
}

//
// Standard instance
//
SemanticParser::SemanticParser(TermBuffer *buffer, Region *output, LocalContext context,
                               LocalObject theirs) :
    QObject(buffer->term()),
    m_buffer(buffer),
    m_region(output),
    m_start(output->startRow),
    m_end(INVALID_INDEX)
{
    m_context.Reset(i, context);
    m_theirs.Reset(i, theirs);

    auto ourstmpl = Local<ObjectTemplate>::New(i, s_processtmpl);
    auto ours = ourstmpl->NewInstance();
    ours->SetAlignedPointerInInternalField(0, this);
    m_ours.Reset(i, ours);
}

void
SemanticParser::destroy()
{
    if (m_timerId)
        killTimer(m_timerId);

    deleteLater();
}

void
SemanticParser::stop()
{
    if (!m_finished) {
        m_populating = false;
        m_parsing = false;
        m_finished = true;

        if (m_timerId) {
            killTimer(m_timerId);
            m_timerId = 0;
        }

        m_ranges.clear();
        m_content.clear();
        m_content.shrink_to_fit();
    }
}

void
SemanticParser::start()
{
    g_watchdog->begin();

    HandleScope scope(i);
    auto context = Local<Context>::New(i, m_context);
    Context::Scope context_scope(context);
    TryCatch trycatch(i);

    auto theirs = Local<Object>::New(i, m_theirs);
    auto ours = Local<Object>::New(i, m_ours);
    Local<Function> func;
    Local<Value> val;
    const Region *job;

    // Start
    val = Local<String>::New(i, s_startkey);
    if (theirs->Get(context, val).ToLocal(&val) && val->IsFunction() &&
        (job = m_buffer->safeRegion(m_region->parent)))
    {
        func = Local<Function>::Cast(val);
        auto exitcode = job->attributes.value(g_attr_REGION_EXITCODE);
        auto exitarg = Integer::NewFromUnsigned(i, exitcode.toUInt());
        auto rowsarg = Integer::NewFromUnsigned(i, m_content.size());

        const int argc = 3;
        Local<Value> argv[argc] = { ours, exitarg, rowsarg };
        g_watchdog->enter();
        bool rc = func->Call(context, theirs, argc, argv).ToLocal(&val);
        g_watchdog->leave();
        if (!rc) {
            Plugin::recover(m_pluginName, m_parserName, "start", trycatch);
            goto stop;
        }
        if (!val->IsTrue())
            goto stop;
    }

    // Process
    val = Local<String>::New(i, s_processkey);
    if (!theirs->Get(context, val).ToLocal(&val) || !val->IsFunction())
        goto stop;
    func = Local<Function>::Cast(val);

    m_size = m_content.size();
    m_parsing = true;

    if (process(context, theirs, ours, func)) {
        // Not done yet
        m_func.Reset(i, func);
        m_timerId = startTimer(0);
        return;
    }
stop:
    stop();
}

void
SemanticParser::timerEvent(QTimerEvent *)
{
    HandleScope scope(i);
    auto context = Local<Context>::New(i, m_context);
    Context::Scope context_scope(context);

    auto theirs = Local<Object>::New(i, m_theirs);
    auto ours = Local<Object>::New(i, m_ours);
    auto func = Local<Function>::New(i, m_func);

    if (!process(context, theirs, ours, func))
        stop();

    m_buffer->buffers()->reportRegionChanged();
}

static inline unsigned
javascriptSize(const std::string &str)
{
    unsigned pos = 0;
    const char *next = str.data(), *end = next + str.size();

    while (next != end) {
        codepoint_t val = utf8::unchecked::next(next);
        pos += 1 + (val > 0xffff);
    }

    return pos;
}

void
SemanticParser::getSpecKeys(LocalString *keys)
{
    keys[Icon] = Local<String>::New(i, s_iconkey);
    keys[Tooltip] = Local<String>::New(i, s_tooltipkey);
    keys[Action1] = Local<String>::New(i, s_action1key);
    keys[Menu] = Local<String>::New(i, s_menukey);
    keys[Drag] = Local<String>::New(i, s_dragkey);
    keys[Uri] = Local<String>::New(i, s_urikey);
}

bool
SemanticParser::process(Local<Context> context, Local<Object> theirs,
                        Local<Object> ours, Local<Function> func)
{
    int iterations = SEMANTIC_BATCH_SIZE;
    std::string str;
    Local<Value> val;
    TryCatch trycatch(i);

    getSpecKeys(m_speckeys);

    while (m_pos < m_size && iterations--)
    {
        str = m_content[m_pos].first;
        m_breaks.clear();
        m_cur = m_start + m_pos;

        auto startarg = Integer::NewFromUnsigned(i, m_pos);

        while (++m_pos < m_size) {
            const auto &cur = m_content[m_pos];
            if (!cur.second)
                break;

            m_breaks.push_back(javascriptSize(m_content[m_pos - 1].first));
            str.append(cur.first);
        }

        const int argc = 4;
        auto endarg = Integer::NewFromUnsigned(i, m_pos - 1);
        auto textarg = v8str(str);
        Local<Value> argv[argc] = { ours, textarg, startarg, endarg };

        g_watchdog->enter();
        bool rc = func->Call(context, theirs, argc, argv).ToLocal(&val);
        g_watchdog->leave();
        if (!rc) {
            Plugin::recover(m_pluginName, m_parserName, "process", trycatch);
            return false;
        }
        if (!val->IsTrue())
            return false;
    }
    if (m_pos < m_size) {
        // Not done yet
        return true;
    }

    // Finish
    val = Local<String>::New(i, s_finishkey);
    if (theirs->Get(context, val).ToLocal(&val) && val->IsFunction())
    {
        func = Local<Function>::Cast(val);
        auto rowsarg = Integer::NewFromUnsigned(i, m_size);

        const int argc = 2;
        Local<Value> argv[argc] = { ours, rowsarg };
        g_watchdog->enter();
        bool rc = func->Call(context, theirs, argc, argv).ToLocal(&val);
        g_watchdog->leave();
        if (!rc) {
            Plugin::recover(m_pluginName, m_parserName, "finish", trycatch);
        }
    }

    return false;
}

inline bool
SemanticParser::checkStart()
{
    if (m_region->flags & Tsq::HasEnd && m_ranges.size() == 1) {
        auto i = m_ranges.cbegin();
        if (i->first == m_start && i->second == m_region->endRow)
        {
            m_populating = false;
            start();
            return true;
        }
    }
    return false;
}

bool
SemanticParser::setRegion()
{
    if (m_region->flags & Tsq::Overwritten || m_region->startRow != m_start ||
        (m_end != INVALID_INDEX && m_region->endRow != m_end))
    {
        stop();
        return false;
    }
    if (m_populating && m_region->flags & Tsq::HasEnd) {
        m_end = m_region->endRow;
        checkStart();
    }

    return true;
}

inline void
SemanticParser::insertRange(index_t value)
{
    if (m_ranges.empty()) {
        m_ranges.emplace_front(value, value);
        m_rangecache = m_ranges.begin();
        return;
    }
    else if (m_rangecache->second + 1 == value) {
        m_rangecache->second = value;
        auto k = m_rangecache, j = m_ranges.end();
        if (++k != j && k->first == value + 1) {
            m_rangecache->second = k->second;
            m_ranges.erase(k);
        }
        return;
    }
    for (auto i = m_ranges.begin(), j = m_ranges.end(); i != j; ++i)
    {
        if (i->second + 1 == value) {
            i->second = value;
            m_rangecache = i;
            auto k = i;
            if (++k != j && k->first == value + 1) {
                i->second = k->second;
                m_ranges.erase(k);
            }
        }
        else if (i->first > value + 1) {
            m_rangecache = m_ranges.insert(i, std::make_pair(value, value));
        }
        else if (i->second + 1 < value) {
            continue;
        }
        else if (i->first == value + 1) {
            i->first = value;
        }

        return;
    }

    m_ranges.emplace_back(value, value);
    m_rangecache = m_ranges.end();
    --m_rangecache;
}

unsigned
SemanticParser::residualPtr(const CellRow &row) const
{
    auto *unicoding = m_buffer->term()->unicoding();
    const char *i = row.str.data(), *j = i + row.str.size();
    column_t pos = 0;

    while (i != j && pos < m_region->endCol) {
        unicoding->next(i, j);
        ++pos;
    }

    return i - row.str.data();
}

bool
SemanticParser::setRow(index_t index, const CellRow &row)
{
    insertRange(index);

    bool continuation = row.flags & Tsq::Continuation;
    auto elt = std::make_pair(std::string(), continuation);

    if (index != m_region->endRow) {
        elt.first = row.str;
    }
    else if (m_region->endCol) {
        elt.first = row.str.substr(0, residualPtr(row));
    }

    index -= m_start;

    if (index > m_content.size())
        m_content.resize(index);
    if (index == m_content.size())
        m_content.push_back(std::move(elt));
    else
        m_content[index] = std::move(elt);

    return checkStart();
}

void
SemanticParser::parseRegionSpec(LocalObject spec, Region *r, LocalString *keys)
{
    auto context = i->GetCurrentContext();
    Local<Value> val;

    // Input menu
    if (spec->Get(context, keys[Menu]).ToLocal(&val) && val->IsObject())
    {
        auto array = Local<Object>::Cast(val);
        QStringList menu;
        for (unsigned i = 0; i < SEMANTIC_MENU_MAX; ++i) {
            if (!array->Get(context, i).ToLocal(&val) || !val->IsObject())
                break;

            auto item = Local<Object>::Cast(val);
            if (!item->Get(context, 0).ToLocal(&val))
                continue;

            menu.append(QString::number(val->Uint32Value()));

            for (unsigned j = 1; j < 5; ++j) {
                QString str;
                if (item->Get(context, j).ToLocal(&val)) {
                    String::Utf8Value utf8(val);
                    str = *utf8;
                }
                menu.append(str);
            }
        }
        r->attributes[g_attr_SEM_MENU] = menu.join('\0');
    }

    // Input drag
    if (spec->Get(context, keys[Drag]).ToLocal(&val) && val->IsObject())
    {
        auto obj = Local<Object>::Cast(val);
        Local<Array> keys = obj->GetOwnPropertyNames();
        QStringList drag;
        for (unsigned i = 0, n = keys->Length(); i < n; ++i) {
            if (!keys->Get(context, i).ToLocal(&val))
                continue;
            String::Utf8Value kutf8(val);
            if (!obj->Get(context, val).ToLocal(&val))
                continue;
            drag.append(*kutf8);
            drag.append(*String::Utf8Value(val));
        }
        r->attributes[g_attr_SEM_DRAG] = drag.join('\0');
    }

    // Input action1
    if (spec->Get(context, keys[Action1]).ToLocal(&val) && val->IsString()) {
        r->attributes[g_attr_SEM_ACTION1] = *String::Utf8Value(val);
    }
    // Input tooltip
    if (spec->Get(context, keys[Tooltip]).ToLocal(&val) && val->IsString()) {
        r->attributes[g_attr_SEM_TOOLTIP] = *String::Utf8Value(val);
    }
    // Input icon
    if (spec->Get(context, keys[Icon]).ToLocal(&val) && val->IsString()) {
        r->attributes[g_attr_SEM_ICON] = *String::Utf8Value(val);
    }
    // Input uri
    if (spec->Get(context, keys[Uri]).ToLocal(&val) && val->IsString()) {
        r->attributes[g_attr_CONTENT_URI] = *String::Utf8Value(val);
    }
}

regionid_t
SemanticParser::createRegion(LocalValue spec, unsigned startCol, unsigned endCol)
{
    if (startCol >= endCol || m_pos == m_size)
        return INVALID_REGION_ID;

    regionid_t id = m_buffer->buffers()->nextSemanticId();
    Region *r = new Region(Tsqt::RegionSemantic, m_buffer, id);
    r->parent = m_region->id();
    r->flags = Tsq::HasStart|Tsq::HasEnd|Tsqt::Updating|Tsqt::Inline;
    r->parser = this;

    index_t row = m_cur;
    for (size_t i = 0; i < m_breaks.size() && startCol >= m_breaks[i]; ++i) {
        ++row;
        startCol -= m_breaks[i];
    }
    r->startRow = row;
    r->startCol = m_buffer->xByJavascript(row, startCol);

    row = m_cur;
    for (size_t i = 0; i < m_breaks.size() && endCol > m_breaks[i]; ++i) {
        ++row;
        endCol -= m_breaks[i];
    }
    r->endRow = row;
    r->endCol = m_buffer->xByJavascript(row, endCol);

    if (spec->IsObject())
        parseRegionSpec(Local<Object>::Cast(spec), r, m_speckeys);

    m_buffer->insertSemanticRegion(r);
    return r->id();
}

regionid_t
SemanticParser::createRegion(LocalValue spec, size_t start, unsigned startCol,
                             size_t end, unsigned endCol)
{
    // Find the starting and ending position
    while (1) {
        if (start >= m_size)
            return INVALID_REGION_ID;
        const auto &row = m_content[start];
        unsigned rowlength = javascriptSize(row.first);
        if (startCol < rowlength || !row.second)
            break;
        startCol -= rowlength;
        ++start;
    }
    while (1) {
        if (end >= m_size)
            return INVALID_REGION_ID;
        const auto &row = m_content[end];
        unsigned rowlength = javascriptSize(row.first);
        if (endCol <= rowlength || !row.second)
            break;
        endCol -= rowlength;
        ++end;
    }

    if (end + 1 == m_size) // squash last row
        endCol = 0;
    if (start > end || (start == end && startCol >= endCol))
        return INVALID_REGION_ID;

    regionid_t id = m_buffer->buffers()->nextSemanticId();
    Region *r = new Region(Tsqt::RegionSemantic, m_buffer, id);
    r->parent = m_region->id();
    r->flags = Tsq::HasStart|Tsq::HasEnd|Tsqt::Updating|Tsqt::Inline;
    r->parser = this;
    r->startRow = m_start + start;
    r->startCol = m_buffer->xByJavascript(r->startRow, startCol);
    r->endRow = m_start + end;
    r->endCol = m_buffer->xByJavascript(r->endRow, endCol);

    if (spec->IsObject())
        parseRegionSpec(Local<Object>::Cast(spec), r, m_speckeys);

    m_buffer->insertSemanticRegion(r);
    return r->id();
}

regionid_t
SemanticParser::peekRegionId() const
{
    return m_buffer->buffers()->peekSemanticId();
}

//
// Fast instance
//
FastSemanticParser::FastSemanticParser(TermBuffer *buffer, Region *output,
                                       LocalContext context, LocalObject theirs) :
    SemanticParser(buffer, output, context, theirs)
{
    Local<Value> val = Local<String>::New(i, s_processkey);
    if (theirs->Get(context, val).ToLocal(&val) && val->IsFunction()) {
        m_func.Reset(i, Local<Function>::Cast(val));
        m_parsing = true;
    } else {
        stop();
    }
}

void
FastSemanticParser::process(size_t limit)
{
    HandleScope outer(i);
    auto context = Local<Context>::New(i, m_context);
    Context::Scope context_scope(context);
    TryCatch trycatch(i);
    Local<Value> val;

    getSpecKeys(m_speckeys);

    auto theirs = Local<Object>::New(i, m_theirs);
    auto ours = Local<Object>::New(i, m_ours);
    auto func = Local<Function>::New(i, m_func);

    // Process
    while (m_pos < limit) {
        HandleScope inner(i);
        std::string str = m_content[m_pos].first;
        m_breaks.clear();
        m_cur = m_start + m_pos;

        auto startarg = Integer::NewFromUnsigned(i, m_pos);

        while (++m_pos < limit) {
            const auto &cur = m_content[m_pos];
            if (!cur.second)
                break;

            m_breaks.push_back(javascriptSize(m_content[m_pos - 1].first));
            str.append(cur.first);
        }

        const int argc = 4;
        auto endarg = Integer::NewFromUnsigned(i, m_pos - 1);
        auto textarg = v8str(str);
        Local<Value> argv[argc] = { ours, textarg, startarg, endarg };

        g_watchdog->enter();
        bool rc = func->Call(context, theirs, argc, argv).ToLocal(&val);
        g_watchdog->leave();
        if (!rc) {
            Plugin::recover(m_pluginName, m_parserName, "process", trycatch);
            goto stop;
        }
        if (!val->IsTrue())
            goto stop;
    }

    if (limit == m_size) {
        // Finish
        val = Local<String>::New(i, s_finishkey);
        if (theirs->Get(context, val).ToLocal(&val) && val->IsFunction())
        {
            func = Local<Function>::Cast(val);
            auto rowsarg = Integer::NewFromUnsigned(i, m_size);

            const int argc = 2;
            Local<Value> argv[argc] = { ours, rowsarg };
            g_watchdog->enter();
            bool rc = func->Call(context, theirs, argc, argv).ToLocal(&val);
            g_watchdog->leave();
            if (!rc) {
                Plugin::recover(m_pluginName, m_parserName, "finish", trycatch);
            }
        }
    stop:
        stop();
    }
}

bool
FastSemanticParser::checkProcess()
{
    auto i = m_ranges.cbegin();

    if (i->first != m_start) {
        return false;
    }
    if (i->second == m_end) {
        process(m_size);
        return true;
    }

    size_t pos = i->second - m_start;
    while (m_content[pos].second) {
        if (pos == 0)
            break;
        --pos;
    }

    if (pos > m_pos) {
        process(pos);
        return true;
    }

    return false;
}

bool
FastSemanticParser::setRegion()
{
    if (m_region->flags & Tsq::Overwritten || m_region->startRow != m_start ||
        (m_end != INVALID_INDEX && m_region->endRow != m_end))
    {
        stop();
        return false;
    }
    if (m_region->flags & Tsq::HasEnd) {
        m_end = m_region->endRow;
        if (!m_ranges.empty())
            checkProcess();
    }

    return true;
}

bool
FastSemanticParser::setRow(index_t index, const CellRow &row)
{
    insertRange(index);

    bool continuation = row.flags & Tsq::Continuation;
    auto elt = std::make_pair(std::string(), continuation);

    if (index != m_region->endRow) {
        elt.first = row.str;
    }
    else if (m_region->endCol) {
        elt.first = row.str.substr(0, residualPtr(row));
    }

    index -= m_start;

    if (index > m_size)
        m_content.resize(index);
    if (index == m_content.size()) {
        m_content.push_back(std::move(elt));
        m_size = index + 1;
    } else {
        m_content[index] = std::move(elt);
    }

    return checkProcess();
}
