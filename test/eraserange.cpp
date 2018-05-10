// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

/*
 * Erase range tests
 */
static void beforeRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);
    LOAD_RANGES(2, 2, 1, 0, 0, 0);

    row.erase(0, 1, wl);

    assert_string_equal(row.str().c_str(), " bc");
    ASSERT_RANGES(2, 2, 1, 0, 0, 0);
}

static void afterRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);
    LOAD_RANGES(0, 0, 1, 0, 0, 0);

    row.erase(2, 3, wl);

    assert_string_equal(row.str().c_str(), "ab ");
    ASSERT_RANGES(0, 0, 1, 0, 0, 0);
}

static void frontEraseOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);
    LOAD_RANGES(0, 0, 1, 0, 0, 0);

    row.erase(0, 1, wl);

    assert_string_equal(row.str().c_str(), " bc");
    ASSERT_RANGE_SIZE(0);
}

static void frontEraseOnRangeMulti(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" SW "c", 3, 3);
    LOAD_RANGES(0, 1, 1, 0, 0, 0);

    row.erase(0, 2, wl);

    assert_string_equal(row.str().c_str(), "  c");
    ASSERT_RANGE_SIZE(0);
}

static void frontSplitEraseOnRangeAligned(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW "bc", 3, 4);
    LOAD_RANGES(0, 0, DWF|1, 0, 0, 0);

    row.erase(0, 1, wl);

    assert_string_equal(row.str().c_str(), "  bc");
    ASSERT_RANGES(1, 1, 1, 0, 0, 0);
}

static void frontSplitEraseOnRangeUnaligned(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW "bc", 3, 4);
    LOAD_RANGES(0, 0, DWF|1, 0, 0, 0);

    row.erase(1, 2, wl);

    assert_string_equal(row.str().c_str(), "  bc");
    ASSERT_RANGES(0, 0, 1, 0, 0, 0);
}

static void frontEraseInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);
    LOAD_RANGES(0, 2, 1, 0, 0, 0);

    row.erase(0, 1, wl);

    assert_string_equal(row.str().c_str(), " bc");
    ASSERT_RANGES(1, 2, 1, 0, 0, 0);
}

static void backEraseOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);
    LOAD_RANGES(2, 2, 1, 0, 0, 0);

    row.erase(2, 3, wl);

    assert_string_equal(row.str().c_str(), "ab ");
    ASSERT_RANGE_SIZE(0);
}

static void backEraseOnRangeMulti(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" SW "c", 3, 3);
    LOAD_RANGES(1, 2, 1, 0, 0, 0);

    row.erase(1, 3, wl);

    assert_string_equal(row.str().c_str(), "a  ");
    ASSERT_RANGE_SIZE(0);
}

static void backSplitEraseOnRangeAligned(void**)
{
    DECLARE_VARS;
    LOAD_STR("ab" DW, 3, 4);
    LOAD_RANGES(2, 2, DWF|1, 0, 0, 0);

    row.erase(2, 3, wl);

    assert_string_equal(row.str().c_str(), "ab  ");
    ASSERT_RANGES(3, 3, 1, 0, 0, 0);
}

static void backSplitEraseOnRangeUnaligned(void**)
{
    DECLARE_VARS;
    LOAD_STR("ab" DW, 3, 4);
    LOAD_RANGES(2, 2, DWF|1, 0, 0, 0);

    row.erase(3, 4, wl);

    assert_string_equal(row.str().c_str(), "ab  ");
    ASSERT_RANGES(2, 2, 1, 0, 0, 0);
}

static void backEraseInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);
    LOAD_RANGES(1, 2, 1, 0, 0, 0);

    row.erase(2, 3, wl);

    assert_string_equal(row.str().c_str(), "ab ");
    ASSERT_RANGES(1, 1, 1, 0, 0, 0);
}

static void midEraseSingleOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(HALF_STR "m" HALF_STR, 9, 14);
    LOAD_RANGES(4, 4, 1, 0, 0, 0);

    row.erase(6, 7, wl);

    assert_string_equal(row.str().c_str(), HALF_STR " " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void midEraseDoubleOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);
    LOAD_RANGES(4, 4, DWF|1, 0, 0, 0);

    row.erase(6, 8, wl);

    assert_string_equal(row.str().c_str(), HALF_STR "  " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void midEraseSplitRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("0123456789", 10, 10);
    LOAD_RANGES(0, 9, 1, 6, 7, 8);

    row.erase(6, 8, wl);

    assert_string_equal(row.str().c_str(), "012345  89");
    ASSERT_RANGES(0, 5, 1, 6, 7, 8, 8, 9, 1, 6, 7, 8);
}

static void midEraseOnMultipleRanges(void**)
{
    DECLARE_VARS;
    LOAD_STR("0123456789", 10, 10);
    LOAD_RANGES(1, 2, 1, 0, 0, 0, 3, 5, 2, 0, 0, 0, 6, 7, 3, 0, 0, 0)

    row.erase(0, 9, wl);

    assert_string_equal(row.str().c_str(), "         9");
    ASSERT_RANGE_SIZE(0);
}

static void midEraseBeforeRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("0123456789", 10, 10);
    LOAD_RANGES(0, 3, 1, 0, 0, 0, 4, 5, 2, 0, 0, 0);

    row.erase(0, 4, wl);

    assert_string_equal(row.str().c_str(), "    456789");
    ASSERT_RANGES(4, 5, 2, 0, 0, 0);
}

static void midEraseOnRangeFront(void**)
{
    DECLARE_VARS;
    LOAD_STR("0123456789", 10, 10);
    LOAD_RANGES(1, 5, 2, 7, 8, 9);

    row.erase(0, 4, wl);

    ASSERT_RANGES(4, 5, 2, 7, 8, 9);
}

static void midEraseAfterRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("0123456789", 10, 10);
    LOAD_RANGES(4, 4, 1, 0, 0, 0, 5, 6, 2, 0, 0, 0);

    row.erase(5, 9, wl);

    assert_string_equal(row.str().c_str(), "01234    9");
    ASSERT_RANGES(4, 4, 1, 0, 0, 0);
}

static void midEraseOnRangeEnd(void**)
{
    DECLARE_VARS;
    LOAD_STR("0123456789", 10, 10);
    LOAD_RANGES(4, 8, 2, 7, 8, 9);

    row.erase(5, 9, wl);

    ASSERT_RANGES(4, 4, 2, 7, 8, 9);
}

int main()
{
    const CMUnitTest tests[] = {
        cmocka_unit_test(beforeRange),
        cmocka_unit_test(afterRange),
        cmocka_unit_test(frontEraseOnRange),
        cmocka_unit_test(frontEraseOnRangeMulti),
        cmocka_unit_test(frontSplitEraseOnRangeAligned),
        cmocka_unit_test(frontSplitEraseOnRangeUnaligned),
        cmocka_unit_test(frontEraseInRange),
        cmocka_unit_test(backEraseOnRange),
        cmocka_unit_test(backEraseOnRangeMulti),
        cmocka_unit_test(backSplitEraseOnRangeAligned),
        cmocka_unit_test(backSplitEraseOnRangeUnaligned),
        cmocka_unit_test(backEraseInRange),
        cmocka_unit_test(midEraseSingleOnRange),
        cmocka_unit_test(midEraseDoubleOnRange),
        cmocka_unit_test(midEraseSplitRange),
        cmocka_unit_test(midEraseOnMultipleRanges),
        cmocka_unit_test(midEraseBeforeRange),
        cmocka_unit_test(midEraseOnRangeFront),
        cmocka_unit_test(midEraseAfterRange),
        cmocka_unit_test(midEraseOnRangeEnd),
    };

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
