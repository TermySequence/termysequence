// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

/*
 * Erase non-range tests
 */
static void frontErase(void**)
{
    DECLARE_VARS;
    LOAD_STR("b" CMB FULL_STR, 10, 15);

    row.erase(0, 1, wl);

    assert_int_equal(row.columns(), 15);
    assert_int_equal(t.clusters(), 10);
    assert_string_equal(row.str().c_str(), " " FULL_STR);
    ASSERT_RANGE_SIZE(0);
}

static void frontSplitErase(void**)
{
    DECLARE_VARS;
    LOAD_STR(DW CMB FULL_STR, 10, 16);

    row.erase(0, 1, wl);

    assert_int_equal(row.columns(), 16);
    assert_int_equal(t.clusters(), 11);
    assert_string_equal(row.str().c_str(), "  " FULL_STR);
    ASSERT_RANGE_SIZE(0);
}

static void frontMulti(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    row.erase(0, 6, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 11);
    assert_string_equal(row.str().c_str(), "      " MID_STR HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void frontMultiSplitEnd(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    row.erase(0, 7, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 12);
    assert_string_equal(row.str().c_str(), "      " "  " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void backErase(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR "b", 10, 15);

    row.erase(14, 15, wl);

    assert_int_equal(row.columns(), 15);
    assert_int_equal(t.clusters(), 10);
    assert_string_equal(row.str().c_str(), FULL_STR " ");
    ASSERT_RANGE_SIZE(0);
}

static void backSplitErase(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR DW, 10, 16);

    row.erase(15, 16, wl);

    assert_int_equal(row.columns(), 16);
    assert_int_equal(t.clusters(), 11);
    assert_string_equal(row.str().c_str(), FULL_STR "  ");
    ASSERT_RANGE_SIZE(0);
}

static void backMulti(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    row.erase(8, 14, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 11);
    assert_string_equal(row.str().c_str(), HALF_STR MID_STR "      ");
    ASSERT_RANGE_SIZE(0);
}

static void backMultiSplitFront(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    row.erase(7, 14, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 12);
    assert_string_equal(row.str().c_str(), HALF_STR "  " "      ");
    ASSERT_RANGE_SIZE(0);
}

static void midSingleWithCombiner(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    row.erase(8, 9, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 9);
    assert_string_equal(row.str().c_str(), HALF_STR MID_STR " " DW CMB DW CMB "b");
    ASSERT_RANGE_SIZE(0);
}

static void midSingle(void**)
{
    DECLARE_VARS;
    LOAD_STR(HALF_STR "z" HALF_STR, 9, 13);

    row.erase(6, 7, wl);

    assert_int_equal(row.columns(), 13);
    assert_int_equal(t.clusters(), 9);
    assert_string_equal(row.str().c_str(), HALF_STR " " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void midDoubleWithCombiner(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    row.erase(6, 8, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 10);
    assert_string_equal(row.str().c_str(), HALF_STR "  " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void midDouble(void**)
{
    DECLARE_VARS;
    LOAD_STR(HALF_STR DW HALF_STR, 9, 14);

    row.erase(6, 8, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 10);
    assert_string_equal(row.str().c_str(), HALF_STR "  " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void midDoubleAlignedSplit(void**)
{
    DECLARE_VARS;
    LOAD_STR(HALF_STR DW HALF_STR, 9, 14);

    row.erase(6, 7, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 10);
    assert_string_equal(row.str().c_str(), HALF_STR "  " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void midDoubleUnalignedSplit(void**)
{
    DECLARE_VARS;
    LOAD_STR(HALF_STR DW HALF_STR, 9, 14);

    row.erase(7, 8, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 10);
    assert_string_equal(row.str().c_str(), HALF_STR "  " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void midSingleSplitDouble(void**)
{
    DECLARE_VARS;
    LOAD_STR(HALF_STR DW HALF_STR, 9, 14);

    row.erase(5, 7, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 10);
    assert_string_equal(row.str().c_str(), "a" CMB DW CMB DW CMB "   " HALF_STR);
    ASSERT_RANGE_SIZE(0);
}

static void midSplitDoubleSingle(void**)
{
    DECLARE_VARS;
    LOAD_STR(HALF_STR DW HALF_STR, 9, 14);

    row.erase(7, 9, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 10);
    assert_string_equal(row.str().c_str(), HALF_STR "   " DW CMB DW CMB "b");
    ASSERT_RANGE_SIZE(0);
}

static void midSplitTwoDoubles(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    row.erase(2, 12, wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 14);
    assert_string_equal(row.str().c_str(), "a" CMB "            b");
    ASSERT_RANGE_SIZE(0);
}

static void fullErase(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    row.erase(0, row.columns(), wl);

    assert_int_equal(row.columns(), 14);
    assert_int_equal(t.clusters(), 14);
    assert_string_equal(row.str().c_str(), "              ");
    //                                      01234567890123
    ASSERT_RANGE_SIZE(0);
}

static void zeroWidthFront(void**)
{
    DECLARE_VARS;
    LOAD_STR("z", 1, 1);

    row.erase(0, 0, wl);

    assert_int_equal(row.columns(), 1);
    assert_int_equal(t.clusters(), 1);
    assert_string_equal(row.str().c_str(), "z");
    ASSERT_RANGE_SIZE(0);
}

static void zeroWidthBack(void**)
{
    DECLARE_VARS;
    LOAD_STR("Z", 1, 2);

    row.erase(2, 2, wl);

    assert_int_equal(row.columns(), 2);
    assert_int_equal(t.clusters(), 1);
    assert_string_equal(row.str().c_str(), "Z");
    ASSERT_RANGE_SIZE(0);
}

static void zeroWidthOffEnd(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);

    row.erase(5, 10, wl);

    assert_int_equal(row.columns(), 3);
    assert_int_equal(t.clusters(), 3);
    assert_string_equal(row.str().c_str(), "abc");
    ASSERT_RANGE_SIZE(0);
}

static void partialOffEnd(void**)
{
    DECLARE_VARS;
    LOAD_STR("abc", 3, 3);

    row.erase(2, 10, wl);

    assert_int_equal(row.columns(), 3);
    assert_int_equal(t.clusters(), 3);
    assert_string_equal(row.str().c_str(), "ab ");
    ASSERT_RANGE_SIZE(0);
}

int main()
{
    REGISTER_UNIPLUGIN(uniplugin_termy_init);

    const CMUnitTest tests[] = {
        cmocka_unit_test(frontErase),
        cmocka_unit_test(frontSplitErase),
        cmocka_unit_test(frontMulti),
        cmocka_unit_test(frontMultiSplitEnd),
        cmocka_unit_test(backErase),
        cmocka_unit_test(backSplitErase),
        cmocka_unit_test(backMulti),
        cmocka_unit_test(backMultiSplitFront),
        cmocka_unit_test(midSingleWithCombiner),
        cmocka_unit_test(midSingle),
        cmocka_unit_test(midDoubleWithCombiner),
        cmocka_unit_test(midDouble),
        cmocka_unit_test(midDoubleAlignedSplit),
        cmocka_unit_test(midDoubleUnalignedSplit),
        cmocka_unit_test(midSingleSplitDouble),
        cmocka_unit_test(midSplitDoubleSingle),
        cmocka_unit_test(midSplitTwoDoubles),
        cmocka_unit_test(fullErase),
        cmocka_unit_test(zeroWidthFront),
        cmocka_unit_test(zeroWidthBack),
        cmocka_unit_test(zeroWidthOffEnd),
        cmocka_unit_test(partialOffEnd),
    };

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
