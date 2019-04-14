// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

// Note: no dependency on common.h allowed here

#pragma once

/*
 * Utility macros
 */
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))
#define LIT_LEN(x) x, sizeof(x) - 1

// Replacement for qDeleteAll with better type warnings
#define forDeleteAll(c) for (auto *x: c) delete x

#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)

/*
 * Globally included headers
 */
#ifdef __cplusplus
#include <cstddef>
#include <cinttypes>
#include <cstring>
#include <cstdlib>
#include <string>
#include <utility>
#include <iterator>
#else
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#endif

/*
 * Globally imported namespaces
 */
#ifdef __cplusplus
using namespace std::string_literals;
#endif
