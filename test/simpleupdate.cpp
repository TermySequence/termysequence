// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

static void nullUpdate(void**)
{
    DECLARE_CURSOR(0);
    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 0);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), 0);
}

static void simpleSingleByteSingleWidth(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR("a", 1, 1);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), 1);
    assert_int_equal(cursor.flags(), 0);
}

static void simpleMultiByteSingleWidth(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR(SW, 1, 1);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void simpleMultiByteDoubleWidth(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR(DW, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void simpleUnaligned(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR(DW, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), Tsq::OnDoubleRight);
}

static void simpleAligned(void**)
{
    DECLARE_CURSOR(0);
    LOAD_STR(DW, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 0);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), Tsq::OnDoubleLeft);
}

static void simpleSingleWidthCombiner(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR("a" CMB, 1, 1);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void simpleSingleWidthCombiners(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR("a" CMB CMB CMB CMB, 1, 1);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void simpleDoubleWidthCombiner(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR(DW CMB, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void simpleDoubleWidthCombiners(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR(DW CMB CMB CMB CMB, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void simpleOverreach(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR("a", 1, 1);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 2);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void longOverreach(void**)
{
    DECLARE_CURSOR(80);
    LOAD_STR("a" CMB DW CMB "a" CMB DW CMB, 4, 6);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 80);
    assert_int_equal(cursor.pos(), 78);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void graphicEmoji(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR(PEMO, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void alignedGraphicEmoji(void**)
{
    DECLARE_CURSOR(0);
    LOAD_STR(PEMO, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 0);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), Tsq::OnDoubleLeft);
}

static void unalignedGraphicEmoji(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR(PEMO, 1, 1);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), Tsq::OnDoubleRight);
}

static void textEmoji(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR(TEMO, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void alignedTextEmoji(void**)
{
    DECLARE_CURSOR(0);
    LOAD_STR(TEMO, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 0);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), Tsq::OnDoubleLeft);
}

static void unalignedTextEmoji(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR(TEMO, 1, 1);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), Tsq::OnDoubleRight);
}

static void zwjEmoji(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR(PEMO ECMB, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void alignedZwjEmoji(void**)
{
    DECLARE_CURSOR(0);
    LOAD_STR(PEMO ECMB, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 0);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), Tsq::OnDoubleLeft);
}

static void unalignedZwjEmoji(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR(PEMO ECMB, 1, 1);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), Tsq::OnDoubleRight);
}

static void modEmoji(void**)
{
    DECLARE_CURSOR(2);
    LOAD_STR(MEMO, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 2);
    assert_int_equal(cursor.pos(), 1);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

static void alignedModEmoji(void**)
{
    DECLARE_CURSOR(0);
    LOAD_STR(MEMO, 1, 2);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 0);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), Tsq::OnDoubleLeft);
}

static void unalignedModEmoji(void**)
{
    DECLARE_CURSOR(1);
    LOAD_STR(MEMO, 1, 1);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 1);
    assert_int_equal(cursor.pos(), 0);
    assert_int_equal(cursor.ptr(), 0);
    assert_int_equal(cursor.flags(), Tsq::OnDoubleRight);
}

static void emojiOverreach(void**)
{
    DECLARE_CURSOR(80);
    LOAD_STR(PEMO ECMB TEMO ECMB MEMO ECMB, 3, 6);

    row.updateCursor(cursor, wl);

    assert_int_equal(cursor.x(), 80);
    assert_int_equal(cursor.pos(), 77);
    assert_int_equal(cursor.ptr(), strlen(STR));
    assert_int_equal(cursor.flags(), 0);
}

int main()
{
    const CMUnitTest tests[] = {
        cmocka_unit_test(nullUpdate),
        cmocka_unit_test(simpleSingleByteSingleWidth),
        cmocka_unit_test(simpleMultiByteSingleWidth),
        cmocka_unit_test(simpleMultiByteDoubleWidth),
        cmocka_unit_test(simpleUnaligned),
        cmocka_unit_test(simpleAligned),
        cmocka_unit_test(simpleSingleWidthCombiner),
        cmocka_unit_test(simpleSingleWidthCombiners),
        cmocka_unit_test(simpleDoubleWidthCombiner),
        cmocka_unit_test(simpleDoubleWidthCombiners),
        cmocka_unit_test(simpleOverreach),
        cmocka_unit_test(longOverreach),
        cmocka_unit_test(graphicEmoji),
        cmocka_unit_test(alignedGraphicEmoji),
        cmocka_unit_test(unalignedGraphicEmoji),
        cmocka_unit_test(textEmoji),
        cmocka_unit_test(alignedTextEmoji),
        cmocka_unit_test(unalignedTextEmoji),
        cmocka_unit_test(zwjEmoji),
        cmocka_unit_test(alignedZwjEmoji),
        cmocka_unit_test(unalignedZwjEmoji),
        cmocka_unit_test(modEmoji),
        cmocka_unit_test(alignedModEmoji),
        cmocka_unit_test(unalignedModEmoji),
        cmocka_unit_test(emojiOverreach),
    };

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
