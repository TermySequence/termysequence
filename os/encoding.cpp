// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "encoding.h"
#include "locale.h"
#include "sysinfo.h"

#include <pthread.h>

std::vector<UnicodingVariant> TermUnicoding::s_variants;

struct ThreadArg {
    uint64_t locale;
    const std::string *spec;
    TermUnicoding *thiz;
    void (TermUnicoding::*populate)(const std::string *);
};

extern "C" void* threadFunc(void *argPtr) {
    const ThreadArg *arg = static_cast<ThreadArg*>(argPtr);

    osSetThreadLocale(arg->locale);
    (arg->thiz->*arg->populate)(arg->spec);
    return NULL;
}

static void
addPlatformParams(std::string &spec, const char *lang)
{
    char buf[16];

    spec.append(LIT_LEN("\x1f" TSQ_UNICODE_PARAM_LOCALE));
    spec.push_back('=');
    spec.append(lang);

    spec.append(LIT_LEN("\x1f" TSQ_UNICODE_PARAM_STDLIB));
    spec.push_back('=');
    spec.append(osGetStdlibName(buf));
    spec.push_back(':');
    spec.append(osGetStdlibVersion(buf));
}

TermUnicoding::TermUnicoding(std::string &&spec, std::string &&lang) :
    m_spec(std::move(spec)),
    m_lang(std::move(lang))
{
    const std::string *specptr = &m_spec;
    std::string newspec;

    if (needsLocale(m_spec.c_str()))
    {
        const char *wantlang = m_lang.c_str();
        const char *curlang = osGetLocale();

        if (strcmp(wantlang, curlang))
            if (!(m_locale = osCreateLocale(wantlang)))
                wantlang = curlang;

        addPlatformParams(newspec = m_spec, wantlang);
        specptr = &newspec;
    }

    ThreadArg arg = { m_locale, specptr, this, &TermUnicoding::populate };
    pthread_t tid;

    if (m_locale && pthread_create(&tid, NULL, &threadFunc, &arg) == 0) {
        pthread_join(tid, NULL);
    } else {
        populate(specptr);
    }
}

TermUnicoding::TermUnicoding(const std::string &spec)
{
    const std::string *specptr = &spec;
    std::string newspec;

    if (needsLocale(spec.c_str())) {
        addPlatformParams(newspec = spec, osGetLocale());
        specptr = &newspec;
    }

    populate(specptr);
}

TermUnicoding::TermUnicoding()
{
    Tsq::UnicodingSpec spec(TSQ_UNICODE_DEFAULT);
    const auto &variant = s_variants.front();
    variant.create(UNIPLUGIN_VERSION, &spec, variant.privarg, this);
}

TermUnicoding::~TermUnicoding()
{
    if (m_locale)
        osFreeLocale(m_locale);
}

void
TermUnicoding::setLocale()
{
    if (m_locale)
        osSetThreadLocale(m_locale);
}

void
TermUnicoding::populate(const std::string *specptr)
{
    Tsq::UnicodingSpec spec(*specptr);

    for (auto i = s_variants.crbegin(), j = s_variants.crend(); i != j; ++i)
        if (!strncmp(spec.variant, i->prefix, i->flags >> 32))
            if ((*i->create)(UNIPLUGIN_VERSION, &spec, i->privarg, this) == 0)
                return;

    // fallback
    const auto &variant = s_variants.front();
    variant.create(UNIPLUGIN_VERSION, &spec, variant.privarg, this);
}

bool
TermUnicoding::needsLocale(const char *prefix)
{
    for (auto i = s_variants.crbegin(), j = s_variants.crend(); i != j; ++i)
        if (!strncmp(prefix, i->prefix, i->flags >> 32))
            if (i->flags & VFPlatformDependent)
                return true;

    return false;
}

void
TermUnicoding::registerPlugin(UnicodingInitFunc func)
{
    UnicodingInfo info;
    if ((*func)(UNIPLUGIN_VERSION, &info) == UNIPLUGIN_VERSION) {
        for (const auto *prec = info.variants; prec->prefix; ++prec) {
            auto &rec = s_variants.emplace_back(*prec);
            // Store the prefix length to avoid strlen later
            rec.flags |= strlen(prec->prefix) << 32;
        }
    }
}
