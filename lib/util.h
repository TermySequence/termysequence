// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

// Note: no dependency on common.h allowed here

#pragma once

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))

// Replacement for qDeleteAll with better type warnings
#define forDeleteAll(c) for (auto *x: c) delete x

#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)
