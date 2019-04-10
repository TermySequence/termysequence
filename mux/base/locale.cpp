// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "locale.h"
#include "os/locale.h"
#include "lib/unicode.h"

#include <pthread.h>

TermLocale::TermLocale(std::string &&spec, std::string &&lang) :
    m_spec(std::move(spec)),
    m_lang(std::move(lang))
{
    m_unicoding = createEncoding();
}

TermLocale::~TermLocale()
{
    delete m_unicoding;

    if (m_locale)
        osFreeLocale(m_locale);
}

void
TermLocale::setLocale()
{
    if (m_locale)
        osSetThreadLocale(m_locale);
}

struct ThreadArg {
    uint64_t locale;
    Tsq::UnicodingSpec *spec;
};

extern "C" void* threadFunc(void *argPtr) {
    const ThreadArg *arg = static_cast<ThreadArg*>(argPtr);

    osSetThreadLocale(arg->locale);
    return Tsq::Unicoding::create(*arg->spec);
}

Tsq::Unicoding *
TermLocale::createEncoding()
{
    Tsq::UnicodingSpec spec(m_spec);
    bool need = Tsq::Unicoding::needsLocale(spec);

    if (need && (m_locale = osCreateLocale(m_lang.c_str())))
    {
        ThreadArg arg = { m_locale, &spec };
        pthread_t tid;

        if (pthread_create(&tid, NULL, &threadFunc, &arg) == 0) {
            void *result;
            pthread_join(tid, &result);
            return static_cast<Tsq::Unicoding*>(result);
        }
    }

    return Tsq::Unicoding::create(spec);
}
