// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

// Mock classes
#include "mockemulator.h"

// Test Macros
#define DECLARE_ROW CellRow row; TermEventTransfer t(row)
#define DECLARE_VARS DECLARE_ROW; auto *wl = new TermUnicoding()
#define DECLARE_CURSOR(x) DECLARE_VARS; Cursor cursor(x)
#define LOAD_STR(str, clu, wid) const char *STR = str; t.setStr(STR, clu, wid)
#define STR_ITER(ptr) t.getStrIter(ptr)

#define LOAD_RANGES(list...) { uint32_t _a[] = { list }; t.setRanges(_a, sizeof(_a)/4); }
#define ASSERT_RANGE_SIZE(s) assert_int_equal(t.ranges().size(), s)
#define ASSERT_RANGES(list...) { \
    uint32_t _a[] = { list }; \
    assert_int_equal(t.ranges().size(), sizeof(_a)/4); \
    assert_memory_equal(t.ranges().data(), _a, sizeof(_a)); }

// Test Strings
#define SW   "\xC3\x96"             // LATIN CAPITAL LETTER O WITH DIAERESIS
#define DW   "\xEF\xBF\xA6"         // FULLWIDTH WON SIGN
#define CMB  "\xCC\x80"             // COMBINING GRAVE
#define PEMO "\xF0\x9F\x83\x8F"     // PLAYING CARD BLACK JOKER
#define TEMO "#\xEF\xB8\x8F"        // # + VARIATION SELECTOR
#define ECMB "\xE2\x80\x8D#"        // ZERO WIDTH JOINER + #
// WHITE UP POINTING INDEX + EMOJI MODIFIER FITZPATRICK TYPE 1-2
#define MEMO "\xE2\x98\x9D\xF0\x9F\x8F\xBB"

#define HALF_STR "a" CMB DW CMB DW CMB "b"
#define MID_STR DW CMB CMB CMB
#define FULL_STR HALF_STR MID_STR HALF_STR

#define DWF Tsq::DblWidthChar

extern "C" {
#include <stdarg.h>
#include <setjmp.h>
#include <assert.h>
#include <cmocka.h>
}
