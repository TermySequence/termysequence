// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

static void simpleSingleWidthMerge(void**)
{
    DECLARE_VARS;
    LOAD_STR("aa", 2, 2);

    t.mergeChars(STR_ITER(0), 0, wl);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 1);
    assert_int_equal(row.str().size(), 1);
    assert_string_equal(row.str().c_str(), "a");
    ASSERT_RANGE_SIZE(0);
}

static void simpleEndOfStringMerge(void**)
{
    DECLARE_VARS;
    LOAD_STR("a", 1, 1);

    t.mergeChars(STR_ITER(0), 0, wl);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 1);
    assert_int_equal(row.str().size(), 1);
    assert_string_equal(row.str().c_str(), "a");
    ASSERT_RANGE_SIZE(0);
}

static void simpleUnalignedMerge(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW, 2, 3);

    t.mergeChars(STR_ITER(0), 0, wl);

    assert_int_equal(row.columns(), 3);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "a ");
    ASSERT_RANGE_SIZE(0);
}

static void singleWidthMergeWithCombiners(void**)
{
    DECLARE_VARS;
    LOAD_STR(HALF_STR "a" CMB "b" CMB "c", 7, 9);

    t.mergeChars(STR_ITER(14), 4, wl);

    assert_int_equal(row.columns(), 9);
    assert_int_equal(t.clusters(), 6);
    assert_int_equal(row.str().size(), 18);
    assert_string_equal(row.str().c_str(), HALF_STR "a" CMB "c");
    ASSERT_RANGE_SIZE(0);
}

static void endOfStringMergeWithCombiners(void**)
{
    DECLARE_VARS;
    LOAD_STR(HALF_STR "a" CMB, 5, 7);

    t.mergeChars(STR_ITER(14), 4, wl);

    assert_int_equal(row.columns(), 8);
    assert_int_equal(t.clusters(), 5);
    assert_int_equal(row.str().size(), 17);
    assert_string_equal(row.str().c_str(), HALF_STR "a" CMB);
    ASSERT_RANGE_SIZE(0);
}

static void mergeOverRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("abcd", 4, 4);
    LOAD_RANGES(2, 2, 1, 0, 0, 0);

    t.mergeChars(STR_ITER(1), 1, wl);

    assert_string_equal(row.str().c_str(), "abd");
    ASSERT_RANGE_SIZE(0);
}

static void mergeIntoRangeBefore(void**)
{
    DECLARE_VARS;
    LOAD_STR("abcd", 4, 4);
    LOAD_RANGES(0, 2, 2, 0, 0, 0);

    t.mergeChars(STR_ITER(1), 1, wl);

    assert_string_equal(row.str().c_str(), "abd");
    ASSERT_RANGES(0, 1, 2, 0, 0, 0);
}

static void mergeIntoRangeAfter(void**)
{
    DECLARE_VARS;
    LOAD_STR("abcd", 4, 4);
    LOAD_RANGES(2, 3, 2, 0, 0, 0);

    t.mergeChars(STR_ITER(1), 1, wl);

    assert_string_equal(row.str().c_str(), "abd");
    ASSERT_RANGES(2, 2, 2, 0, 0, 0);
}

static void mergeCoalesceRanges(void**)
{
    DECLARE_VARS;
    LOAD_STR("abcd", 4, 4);
    LOAD_RANGES(0, 1, 2, 0, 0, 0, 2, 2, 1, 0, 0, 0, 3, 3, 2, 0, 0, 0);

    t.mergeChars(STR_ITER(1), 1, wl);

    assert_string_equal(row.str().c_str(), "abd");
    ASSERT_RANGES(0, 2, 2, 0, 0, 0);
}

int main()
{
    const CMUnitTest tests[] = {
        cmocka_unit_test(simpleSingleWidthMerge),
        cmocka_unit_test(simpleEndOfStringMerge),
        cmocka_unit_test(simpleUnalignedMerge),
        cmocka_unit_test(singleWidthMergeWithCombiners),
        cmocka_unit_test(endOfStringMergeWithCombiners),
        cmocka_unit_test(mergeOverRange),
        cmocka_unit_test(mergeIntoRangeBefore),
        cmocka_unit_test(mergeIntoRangeAfter),
        cmocka_unit_test(mergeCoalesceRanges),
    };

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
