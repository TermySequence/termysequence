// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

static void simpleSplit(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW, 1, 2);

    assert_int_equal(t.splitChar(STR_ITER(0), 0, wl), 0);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "  ");
    ASSERT_RANGE_SIZE(0);
}

static void simpleSplitWithCombiner(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB, 1, 2);

    assert_int_equal(t.splitChar(STR_ITER(0), 0, wl), 0);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "  ");
    ASSERT_RANGE_SIZE(0);
}

static void simpleSplitWithCombiners(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB CMB CMB CMB, 1, 2);

    assert_int_equal(t.splitChar(STR_ITER(0), 0, wl), 0);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "  ");
    ASSERT_RANGE_SIZE(0);
}

static void midSplit(void**)
{
    DECLARE_VARS;
    LOAD_STR(HALF_STR DW HALF_STR, 9, 14);

    assert_int_equal(t.splitChar(STR_ITER(14), 4, wl), 14);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 10);
    assert_int_equal(row.str().size(), 30);
    assert_string_equal(row.str().c_str(), HALF_STR "  " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void midSplitWithCombiners(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    assert_int_equal(t.splitChar(STR_ITER(14), 4, wl), 14);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 10);
    assert_int_equal(row.str().size(), 30);
    assert_string_equal(row.str().c_str(), HALF_STR "  " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void simpleSplitOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW, 1, 2);
    LOAD_RANGES(0, 0, DWF|1, 0, 0, 0);

    assert_int_equal(t.splitChar(STR_ITER(0), 0, wl), 0);

    ASSERT_RANGES(0, 1, 1, 0, 0, 0);
}

static void splitOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);
    LOAD_RANGES(4, 4, DWF|1, 0, 0, 0);

    assert_int_equal(t.splitChar(STR_ITER(14), 4, wl), 14);

    ASSERT_RANGES(4, 5, 1, 0, 0, 0);
}

static void splitAfterRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);
    LOAD_RANGES(0, 3, 1, 0, 0, 0);

    assert_int_equal(t.splitChar(STR_ITER(14), 4, wl), 14);

    ASSERT_RANGES(0, 3, 1, 0, 0, 0);
}

static void splitBeforeRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);
    LOAD_RANGES(4, 4, DWF, 0, 0, 0, 5, 9, 1, 0, 0, 0);

    assert_int_equal(t.splitChar(STR_ITER(14), 4, wl), 14);

    ASSERT_RANGES(6, 10, 1, 0, 0, 0);
}

static void splitInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);
    LOAD_RANGES(3, 3, 1, 0, 0, 0, 4, 4, DWF|1, 0, 0, 0, 5, 5, 1, 0, 0, 0);

    assert_int_equal(t.splitChar(STR_ITER(14), 4, wl), 14);

    ASSERT_RANGES(3, 6, 1, 0, 0, 0);
}

static void splitBetweenRanges(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);
    LOAD_RANGES(0, 3, 1, 0, 0, 0, 4, 4, DWF, 0, 0, 0, 5, 9, 2, 0, 0, 0);

    assert_int_equal(t.splitChar(STR_ITER(14), 4, wl), 14);

    ASSERT_RANGES(0, 3, 1, 0, 0, 0, 6, 10, 2, 0, 0, 0);
}

int main()
{
    REGISTER_UNIPLUGIN(uniplugin_termy_init);

    const CMUnitTest tests[] = {
        cmocka_unit_test(simpleSplit),
        cmocka_unit_test(simpleSplitWithCombiner),
        cmocka_unit_test(simpleSplitWithCombiners),
        cmocka_unit_test(midSplit),
        cmocka_unit_test(midSplitWithCombiners),
        cmocka_unit_test(simpleSplitOnRange),
        cmocka_unit_test(splitOnRange),
        cmocka_unit_test(splitAfterRange),
        cmocka_unit_test(splitBeforeRange),
        cmocka_unit_test(splitInRange),
        cmocka_unit_test(splitBetweenRanges),
    };

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
