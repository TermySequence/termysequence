// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

static CellAttributes a, d;

/*
 * Replace tests
 */
static void doubleWithSingleAligned(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR("a" DW CMB "c", 3, 4);

    row.updateCursor(cursor, wl);
    assert_int_equal(cursor.pos(), 1);
    row.replace(cursor, a, 'x', 1, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 1);

    assert_int_equal(row.columns(), 4);
    assert_int_equal(t.clusters(), 4);
    assert_int_equal(row.str().size(), 4);
    assert_string_equal(row.str().c_str(), "ax c");
    ASSERT_RANGE_SIZE(0);
}

static void doubleWithSingleUnaligned(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR("a" DW CMB "c", 3, 4);

    row.updateCursor(cursor, wl);
    assert_int_equal(cursor.pos(), 1);
    row.replace(cursor, a, 'x', 1, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 2);

    assert_int_equal(row.columns(), 4);
    assert_int_equal(t.clusters(), 4);
    assert_int_equal(row.str().size(), 4);
    assert_string_equal(row.str().c_str(), "a xc");
    ASSERT_RANGE_SIZE(0);
}

static void twoSinglesWithDouble(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR("abcd", 4, 4);

    row.updateCursor(cursor, wl);
    assert_int_equal(cursor.pos(), 1);
    row.replace(cursor, d, 0xffe6, 2, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 1);

    assert_int_equal(row.columns(), 4);
    assert_int_equal(t.clusters(), 3);
    assert_int_equal(row.str().size(), 5);
    assert_string_equal(row.str().c_str(), "a" DW "d");
    ASSERT_RANGES(1, 1, DWF, 0, 0, 0);
}

static void oneSingleWithDouble(void**)
{
    DECLARE_CURSOR(3);
    LOAD_STR("abcd", 4, 4);

    row.updateCursor(cursor, wl);
    assert_int_equal(cursor.pos(), 3);
    row.replace(cursor, d, 0xffe6, 2, wl);

    assert_int_equal(cursor.x(), 3);
    assert_int_equal(cursor.pos(), 3);

    assert_int_equal(row.columns(), 5);
    assert_int_equal(t.clusters(), 4);
    assert_int_equal(row.str().size(), 6);
    assert_string_equal(row.str().c_str(), "abc" DW );
    ASSERT_RANGES(3, 3, DWF, 0, 0, 0);
}

static void singleAndDoubleWithDouble(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR("ab" DW CMB "d", 4, 5);

    row.updateCursor(cursor, wl);
    assert_int_equal(cursor.pos(), 1);
    row.replace(cursor, d, 0xffe6, 2, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 1);

    assert_int_equal(row.columns(), 5);
    assert_int_equal(t.clusters(), 4);
    assert_int_equal(row.str().size(), 6);
    assert_string_equal(row.str().c_str(), "a" DW " d");
    ASSERT_RANGES(1, 1, DWF, 0, 0, 0);
}

static void doubleAndSingleWithDouble(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR("a" DW CMB "cd", 4, 5);

    row.updateCursor(cursor, wl);
    assert_int_equal(cursor.pos(), 1);
    row.replace(cursor, d, 0xffe6, 2, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 2);

    assert_int_equal(row.columns(), 5);
    assert_int_equal(t.clusters(), 4);
    assert_int_equal(row.str().size(), 6);
    assert_string_equal(row.str().c_str(), "a " DW "d");
    ASSERT_RANGES(2, 2, DWF, 0, 0, 0);
}

static void doubleAndDoubleWithDouble(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR("a" DW CMB DW CMB "d", 4, 6);

    row.updateCursor(cursor, wl);
    assert_int_equal(cursor.pos(), 1);
    row.replace(cursor, d, 0xffe6, 2, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 2);

    assert_int_equal(row.columns(), 6);
    assert_int_equal(t.clusters(), 5);
    assert_int_equal(row.str().size(), 7);
    assert_string_equal(row.str().c_str(), "a " DW " d");
    ASSERT_RANGES(2, 2, DWF, 0, 0, 0);
}

int main()
{
    REGISTER_UNIPLUGIN(uniplugin_termy_init);

    const CMUnitTest tests[] = {
        cmocka_unit_test(doubleWithSingleAligned),
        cmocka_unit_test(doubleWithSingleUnaligned),
        cmocka_unit_test(twoSinglesWithDouble),
        cmocka_unit_test(oneSingleWithDouble),
        cmocka_unit_test(singleAndDoubleWithDouble),
        cmocka_unit_test(doubleAndSingleWithDouble),
        cmocka_unit_test(doubleAndDoubleWithDouble),
    };

    d.flags = DWF;

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
