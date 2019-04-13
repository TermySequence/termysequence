// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "locale.h"

#ifndef __linux__
/* Mac OS X */
#include <xlocale.h>
#endif

uint64_t
osCreateLocale(const char *lang)
{
    locale_t locale = 0;
    if (*lang) {
        locale = duplocale(LC_GLOBAL_LOCALE);
        locale = newlocale(LC_COLLATE_MASK|LC_CTYPE_MASK, lang, locale);
    }
    return reinterpret_cast<uint64_t>(locale);
}

void
osSetThreadLocale(uint64_t locale)
{
    uselocale(reinterpret_cast<locale_t>(locale));
}

void
osFreeLocale(uint64_t locale)
{
    freelocale(reinterpret_cast<locale_t>(locale));
}
