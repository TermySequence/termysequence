// Copyright © 2019 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <locale.h>

#define osInitLocale() setlocale(LC_ALL, "")
#define osGetLocale() setlocale(LC_CTYPE, NULL)
#define osGetLang() setlocale(LC_MESSAGES, NULL)

extern uint64_t
osCreateLocale(const char *lang);

extern void
osSetThreadLocale(uint64_t locale);

extern void
osFreeLocale(uint64_t locale);
