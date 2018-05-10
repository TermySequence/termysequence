// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

static void simpleSingleByteInsert(void**)
{
    DECLARE_VARS;
    LOAD_STR("ac", 2, 2);

    row.insert(1, wl);

    assert_int_equal(row.columns(), 3);
    assert_int_equal(t.clusters(), 3);
    assert_int_equal(row.str().size(), 3);
    assert_string_equal(row.str().c_str(), "a c");
    ASSERT_RANGE_SIZE(0);
}

static void simpleEndOfStringInsert(void**)
{
    DECLARE_VARS;
    LOAD_STR("ab", 2, 2);

    row.insert(2, wl);

    assert_int_equal(row.columns(), 3);
    assert_int_equal(t.clusters(), 3);
    assert_int_equal(row.str().size(), 3);
    assert_string_equal(row.str().c_str(), "ab ");
    ASSERT_RANGE_SIZE(0);
}

static void multiByteInsertBefore(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);

    row.insert(1, wl);

    assert_int_equal(row.columns(), 5);
    assert_int_equal(t.clusters(), 4);
    assert_int_equal(row.str().size(), 10);
    assert_string_equal(row.str().c_str(), "a " DW CMB CMB "c");
    ASSERT_RANGE_SIZE(0);
}

static void multiByteInsertAfter(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);

    row.insert(3, wl);

    assert_int_equal(row.columns(), 5);
    assert_int_equal(t.clusters(), 4);
    assert_int_equal(row.str().size(), 10);
    assert_string_equal(row.str().c_str(), "a" DW CMB CMB " c");
    ASSERT_RANGE_SIZE(0);
}

static void splitInsert(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);

    row.insert(2, wl);

    assert_int_equal(row.columns(), 5);
    assert_int_equal(t.clusters(), 5);
    assert_int_equal(row.str().size(), 5);
    assert_string_equal(row.str().c_str(), "a   c");
    ASSERT_RANGE_SIZE(0);
}

static void insertInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("ac", 2, 2);
    LOAD_RANGES(0, 1, 1, 0, 0, 0);

    row.insert(1, wl);

    assert_string_equal(row.str().c_str(), "a c");
    ASSERT_RANGES(0, 0, 1, 0, 0, 0, 2, 2, 1, 0, 0, 0);
}

static void splitInsertOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);
    LOAD_RANGES(1, 1, DWF|1, 0, 0, 0);

    row.insert(2, wl);

    assert_string_equal(row.str().c_str(), "a   c");
    ASSERT_RANGES(1, 1, 1, 0, 0, 0, 3, 3, 1, 0, 0, 0);
}

static void splitInsertInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);
    LOAD_RANGES(0, 0, 1, 0, 0, 0, 1, 1, DWF|1, 0, 0, 0, 2, 2, 1, 0, 0, 0);

    row.insert(2, wl);

    assert_string_equal(row.str().c_str(), "a   c");
    ASSERT_RANGES(0, 1, 1, 0, 0, 0, 3, 4, 1, 0, 0, 0);
}

static void insertAfterRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" CMB "b" CMB "c" CMB "", 3, 3);
    LOAD_RANGES(0, 0, 1, 0, 0, 0);

    row.insert(1, wl);

    assert_string_equal(row.str().c_str(), "a" CMB " b" CMB "c" CMB "");
    ASSERT_RANGES(0, 0, 1, 0, 0, 0);
}

static void insertBeforeRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" CMB "b" CMB "c" CMB "", 3, 3);
    LOAD_RANGES(1, 2, 1, 0, 0, 0);

    row.insert(1, wl);

    assert_string_equal(row.str().c_str(), "a" CMB " b" CMB "c" CMB "");
    ASSERT_RANGES(2, 3, 1, 0, 0, 0);
}

static void insertBetweenRanges(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB DW CMB, 2, 4);
    LOAD_RANGES(0, 0, DWF|1, 0, 0, 0, 1, 1, DWF|2, 0, 0, 0);

    row.insert(2, wl);

    assert_string_equal(row.str().c_str(), DW CMB " " DW CMB);
    ASSERT_RANGES(0, 0, DWF|1, 0, 0, 0, 2, 2, DWF|2, 0, 0, 0);
}

int main()
{
    const CMUnitTest tests[] = {
        cmocka_unit_test(simpleSingleByteInsert),
        cmocka_unit_test(simpleEndOfStringInsert),
        cmocka_unit_test(multiByteInsertBefore),
        cmocka_unit_test(multiByteInsertAfter),
        cmocka_unit_test(splitInsert),
        cmocka_unit_test(insertInRange),
        cmocka_unit_test(splitInsertOnRange),
        cmocka_unit_test(splitInsertInRange),
        cmocka_unit_test(insertAfterRange),
        cmocka_unit_test(insertBeforeRange),
        cmocka_unit_test(insertBetweenRanges),
    };

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
