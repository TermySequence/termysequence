// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

/*
 * Remove tests
 */
static void simpleSingleByteRemove(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);

    row.remove(1, wl);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "ac");
    ASSERT_RANGE_SIZE(0);
}

static void simpleEndOfStringRemove(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);

    row.remove(2, wl);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "ab");
    ASSERT_RANGE_SIZE(0);
}

static void multiByteRemove(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" SW CMB "c", 3, 3);

    row.remove(1, wl);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "ac");
    ASSERT_RANGE_SIZE(0);
}

static void multiByteEndOfStringRemove(void**)
{
    DECLARE_VARS;
    LOAD_STR("ab" CMB SW CMB CMB, 3, 3);

    row.remove(2, wl);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 4);
    assert_string_equal(row.str().c_str(), "ab" CMB);
    ASSERT_RANGE_SIZE(0);
}

static void alignedRemove(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);

    row.remove(1, wl);

    assert_int_equal(row.columns(), 3);
    assert_int_equal(t.clusters(), 3);
    assert_int_equal(row.str().size(), 3);
    assert_string_equal(row.str().c_str(), "a c");
    ASSERT_RANGE_SIZE(0);
}

static void unalignedRemove(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);

    row.remove(2, wl);

    assert_int_equal(row.columns(), 3);
    assert_int_equal(t.clusters(), 3);
    assert_int_equal(row.str().size(), 3);
    assert_string_equal(row.str().c_str(), "a c");
    ASSERT_RANGE_SIZE(0);
}

/*
 * Single-width range tests
 */
static void simpleRemoveOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a", 1, 1);
    LOAD_RANGES(0, 0, 1, 0, 0, 0);

    row.remove(0, wl);

    assert_string_equal(row.str().c_str(), "");
    ASSERT_RANGE_SIZE(0);
}

static void removeOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" SW CMB CMB "c", 3, 3);
    LOAD_RANGES(1, 1, 1, 0, 0, 0);

    row.remove(1, wl);

    assert_string_equal(row.str().c_str(), "ac");
    ASSERT_RANGE_SIZE(0);
}

static void removeAfterRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" CMB SW CMB CMB "c" CMB, 3, 3);
    LOAD_RANGES(0, 0, 1, 0, 0, 0);

    row.remove(1, wl);

    assert_string_equal(row.str().c_str(), "a" CMB "c" CMB);
    ASSERT_RANGES(0, 0, 1, 0, 0, 0);
}

static void removeBeforeRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" CMB SW CMB CMB "c" CMB, 3, 3);
    LOAD_RANGES(2, 2, 1, 0, 0, 0);

    row.remove(1, wl);

    assert_string_equal(row.str().c_str(), "a" CMB "c" CMB);
    ASSERT_RANGES(1, 1, 1, 0, 0, 0);
}

static void alignedRemoveInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB DW CMB CMB DW CMB, 3, 6);
    LOAD_RANGES(0, 2, DWF|1, 0, 0, 0);

    row.remove(2, wl);

    assert_string_equal(row.str().c_str(), DW CMB " " DW CMB);
    ASSERT_RANGES(0, 0, DWF|1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 2, 2, DWF|1, 0, 0, 0);
}

static void unalignedRemoveInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB DW CMB CMB DW CMB, 3, 6);
    LOAD_RANGES(0, 2, DWF|1, 0, 0, 0);

    row.remove(3, wl);

    assert_string_equal(row.str().c_str(), DW CMB " " DW CMB);
    ASSERT_RANGES(0, 0, DWF|1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 2, 2, DWF|1, 0, 0, 0);
}

static void removeBetweenRanges(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB SW CMB CMB DW CMB, 3, 6);
    LOAD_RANGES(0, 0, DWF|1, 0, 0, 0, 2, 2, DWF|2, 0, 0, 0);

    row.remove(2, wl);

    assert_string_equal(row.str().c_str(), DW CMB DW CMB);
    ASSERT_RANGES(0, 0, DWF|1, 0, 0, 0, 1, 1, DWF|2, 0, 0, 0);
}

static void removeCoalesceRanges(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB SW CMB CMB DW CMB, 3, 6);
    LOAD_RANGES(0, 0, DWF|1, 0, 0, 0, 2, 2, DWF|1, 0, 0, 0);

    row.remove(2, wl);

    assert_string_equal(row.str().c_str(), DW CMB DW CMB);
    ASSERT_RANGES(0, 1, DWF|1, 0, 0, 0);
}

/*
 * Single-width range tests
 */
static void alignedOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);
    LOAD_RANGES(1, 1, DWF|1, 0, 0, 0);

    row.remove(1, wl);

    assert_string_equal(row.str().c_str(), "a c");
    ASSERT_RANGES(1, 1, 1, 0, 0, 0);
}

static void unalignedOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);
    LOAD_RANGES(1, 1, DWF|1, 0, 0, 0);

    row.remove(2, wl);

    assert_string_equal(row.str().c_str(), "a c");
    ASSERT_RANGES(1, 1, 1, 0, 0, 0);
}

static void alignedInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);
    LOAD_RANGES(0, 0, 1, 0, 0, 0, 1, 1, DWF|1, 0, 0, 0, 2, 2, 1, 0, 0, 0);

    row.remove(1, wl);

    assert_string_equal(row.str().c_str(), "a c");
    ASSERT_RANGES(0, 2, 1, 0, 0, 0);
}

static void unalignedInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);
    LOAD_RANGES(0, 0, 1, 0, 0, 0, 1, 1, DWF|1, 0, 0, 0, 2, 2, 1, 0, 0, 0);

    row.remove(2, wl);

    assert_string_equal(row.str().c_str(), "a c");
    ASSERT_RANGES(0, 2, 1, 0, 0, 0);
}

static void alignedBetweenRanges(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);
    LOAD_RANGES(0, 0, 1, 0, 0, 0, 1, 1, DWF, 0, 0, 0, 2, 2, 2, 0, 0, 0);

    row.remove(1, wl);

    assert_string_equal(row.str().c_str(), "a c");
    ASSERT_RANGES(0, 0, 1, 0, 0, 0, 2, 2, 2, 0, 0, 0);
}

static void unalignedBetweenRanges(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);
    LOAD_RANGES(0, 0, 1, 0, 0, 0, 1, 1, DWF, 0, 0, 0, 2, 2, 2, 0, 0, 0);

    row.remove(2, wl);

    assert_string_equal(row.str().c_str(), "a c");
    ASSERT_RANGES(0, 0, 1, 0, 0, 0, 2, 2, 2, 0, 0, 0);
}

int main()
{
    REGISTER_UNIPLUGIN(uniplugin_termy_init);

    const CMUnitTest tests[] = {
        cmocka_unit_test(simpleSingleByteRemove),
        cmocka_unit_test(simpleEndOfStringRemove),
        cmocka_unit_test(multiByteRemove),
        cmocka_unit_test(multiByteEndOfStringRemove),
        cmocka_unit_test(alignedRemove),
        cmocka_unit_test(unalignedRemove),
        cmocka_unit_test(simpleRemoveOnRange),
        cmocka_unit_test(removeOnRange),
        cmocka_unit_test(removeAfterRange),
        cmocka_unit_test(removeBeforeRange),
        cmocka_unit_test(alignedRemoveInRange),
        cmocka_unit_test(unalignedRemoveInRange),
        cmocka_unit_test(removeBetweenRanges),
        cmocka_unit_test(removeCoalesceRanges),
        cmocka_unit_test(alignedOnRange),
        cmocka_unit_test(unalignedOnRange),
        cmocka_unit_test(alignedInRange),
        cmocka_unit_test(unalignedInRange),
        cmocka_unit_test(alignedBetweenRanges),
        cmocka_unit_test(unalignedBetweenRanges),
    };

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
