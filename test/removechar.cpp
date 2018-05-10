// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

/*
 * removeChar tests
 */
static void simpleSingleByteRemove(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);

    t.removeChar(STR_ITER(1), 1, wl);

    assert_int_equal(row.columns(), 3);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "ac");
    ASSERT_RANGE_SIZE(0);
}

static void simpleEndOfStringRemove(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);

    t.removeChar(STR_ITER(2), 2, wl);

    assert_int_equal(row.columns(), 3);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "ab");
    ASSERT_RANGE_SIZE(0);
}

static void multiByteRemove(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB "c", 3, 4);

    t.removeChar(STR_ITER(1), 1, wl);

    assert_int_equal(row.columns(), 4);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "ac");
    ASSERT_RANGE_SIZE(0);
}

static void multiByteEndOfStringRemove(void**)
{
    DECLARE_VARS;
    LOAD_STR("ab" DW CMB CMB, 3, 4);

    t.removeChar(STR_ITER(2), 2, wl);

    assert_int_equal(row.columns(), 4);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "ab");
    ASSERT_RANGE_SIZE(0);
}

static void simpleRemoveOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a", 1, 1);
    LOAD_RANGES(0, 0, 1, 0, 0, 0);

    t.removeChar(STR_ITER(0), 0, wl);

    assert_string_equal(row.str().c_str(), "");
    ASSERT_RANGE_SIZE(0);
}

static void removeOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" SW CMB CMB "c", 3, 3);
    LOAD_RANGES(1, 1, 1, 0, 0, 0);

    t.removeChar(STR_ITER(1), 1, wl);

    assert_string_equal(row.str().c_str(), "ac");
    ASSERT_RANGE_SIZE(0);
}

static void removeAfterRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" CMB SW CMB CMB "c" CMB, 3, 3);
    LOAD_RANGES(0, 0, 1, 0, 0, 0);

    t.removeChar(STR_ITER(3), 1, wl);

    assert_string_equal(row.str().c_str(), "a" CMB "c" CMB);
    ASSERT_RANGES(0, 0, 1, 0, 0, 0);
}

static void removeBeforeRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" CMB SW CMB CMB "c" CMB, 3, 3);
    LOAD_RANGES(2, 2, 1, 0, 0, 0);

    t.removeChar(STR_ITER(3), 1, wl);

    assert_string_equal(row.str().c_str(), "a" CMB "c" CMB);
    ASSERT_RANGES(1, 1, 1, 0, 0, 0);
}

static void removeInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB DW CMB CMB DW CMB, 3, 6);
    LOAD_RANGES(0, 2, DWF|1, 0, 0, 0);

    t.removeChar(STR_ITER(5), 1, wl);

    assert_string_equal(row.str().c_str(), DW CMB DW CMB);
    ASSERT_RANGES(0, 1, DWF|1, 0, 0, 0);
}

static void removeBetweenRanges(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB DW CMB CMB DW CMB, 3, 6);
    LOAD_RANGES(0, 0, DWF|1, 0, 0, 0, 1, 1, DWF, 0, 0, 0, 2, 2, DWF|2, 0, 0, 0);

    t.removeChar(STR_ITER(5), 1, wl);

    assert_string_equal(row.str().c_str(), DW CMB DW CMB);
    ASSERT_RANGES(0, 0, DWF|1, 0, 0, 0, 1, 1, DWF|2, 0, 0, 0);
}

static void removeCoalesceRanges(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB DW CMB CMB DW CMB, 3, 6);
    LOAD_RANGES(0, 0, DWF|1, 0, 0, 0, 1, 1, DWF, 0, 0, 0, 2, 2, DWF|1, 0, 0, 0);

    t.removeChar(STR_ITER(5), 1, wl);

    assert_string_equal(row.str().c_str(), DW CMB DW CMB);
    ASSERT_RANGES(0, 1, DWF|1, 0, 0, 0);
}

/*
 * pop_back tests
 */
static void simplePopBack(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);

    row.resize(2, wl);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "ab");
    ASSERT_RANGE_SIZE(0);
}

static void popBackWithCombiner(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc" CMB, 3, 3);

    row.resize(2, wl);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "ab");
    ASSERT_RANGE_SIZE(0);
}

static void popBackDoubleWidth(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW, 1, 2);

    row.resize(1, wl);

    assert_int_equal(row.columns(), 1);
    assert_int_equal(t.clusters(), 1);
    assert_int_equal(row.str().size(), 1);
    assert_string_equal(row.str().c_str(), " ");
    ASSERT_RANGE_SIZE(0);
}

static void popBackMultiWidth(void**)
{
    DECLARE_VARS;
    LOAD_STR("a" DW CMB CMB, 2, 3);

    row.resize(2, wl);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 2);
    assert_int_equal(row.str().size(), 2);
    assert_string_equal(row.str().c_str(), "a ");
    ASSERT_RANGE_SIZE(0);
}

static void popBackOnRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);
    LOAD_RANGES(0, 1, 2, 0, 0, 0, 2, 2, 1, 0, 0, 0);

    row.resize(2, wl);

    assert_string_equal(row.str().c_str(), "ab");
    ASSERT_RANGES(0, 1, 2, 0, 0, 0);
}

static void popBackInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);
    LOAD_RANGES(0, 0, 2, 0, 0, 0, 1, 2, 1, 0, 0, 0);

    row.resize(2, wl);

    assert_string_equal(row.str().c_str(), "ab");
    ASSERT_RANGES(0, 0, 2, 0, 0, 0, 1, 1, 1, 0, 0, 0);
}

static void popBackDoubleWidthInRange(void**)
{
    DECLARE_VARS;
    LOAD_STR(SW CMB DW, 2, 3);
    LOAD_RANGES(0, 0, 2, 0, 0, 0, 1, 1, DWF|1, 0, 0, 0);

    row.resize(2, wl);

    assert_string_equal(row.str().c_str(), SW CMB " ");
    ASSERT_RANGES(0, 0, 2, 0, 0, 0, 1, 1, 1, 0, 0, 0);
}

int main()
{
    const CMUnitTest tests[] = {
        cmocka_unit_test(simpleSingleByteRemove),
        cmocka_unit_test(simpleEndOfStringRemove),
        cmocka_unit_test(multiByteRemove),
        cmocka_unit_test(multiByteEndOfStringRemove),
        cmocka_unit_test(simpleRemoveOnRange),
        cmocka_unit_test(removeOnRange),
        cmocka_unit_test(removeAfterRange),
        cmocka_unit_test(removeBeforeRange),
        cmocka_unit_test(removeInRange),
        cmocka_unit_test(removeBetweenRanges),
        cmocka_unit_test(removeCoalesceRanges),
        cmocka_unit_test(simplePopBack),
        cmocka_unit_test(popBackWithCombiner),
        cmocka_unit_test(popBackDoubleWidth),
        cmocka_unit_test(popBackMultiWidth),
        cmocka_unit_test(popBackOnRange),
        cmocka_unit_test(popBackInRange),
        cmocka_unit_test(popBackDoubleWidthInRange),
    };

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
