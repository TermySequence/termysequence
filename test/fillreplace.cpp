// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

static CellAttributes a, b, c, d;
static const codepoint_t f = ' ';

/*
 * append tests
 */
static void push1(void**) {
    DECLARE_ROW;
    row.append(a, f, 1);
}

static void push2(void**) {
    DECLARE_ROW;
    row.append(b, f, 1);

    ASSERT_RANGES(0, 0, 1, 0, 0, 0);
}

static void push3(void**) {
    DECLARE_ROW;
    row.append(b, f, 1);
    row.append(b, f, 1);

    ASSERT_RANGES(0, 1, 1, 0, 0, 0);
}

static void push4(void**) {
    DECLARE_ROW;
    row.append(a, f, 1);
    row.append(b, f, 1);

    ASSERT_RANGES(1, 1, 1, 0, 0, 0);
}

static void push5(void**) {
    DECLARE_ROW;
    row.append(b, f, 1);
    row.append(a, f, 1);

    ASSERT_RANGES(0, 0, 1, 0, 0, 0);
}

static void push6(void**) {
    DECLARE_ROW;
    row.append(b, f, 1);
    row.append(a, f, 1);
    row.append(b, f, 1);

    ASSERT_RANGES(0, 0, 1, 0, 0, 0, 2, 2, 1, 0, 0, 0);
}

/*
 * replace tests
 */
static void replace1(void**) {
    DECLARE_CURSOR(1);
    row.append(a, f, 1);
    row.append(a, f, 1);
    row.append(a, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(1, 1, 1, 0, 0, 0);
}

static void replace2(void**) {
    DECLARE_CURSOR(1);
    row.append(b, f, 1);
    row.append(a, f, 1);
    row.append(b, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(0, 2, 1, 0, 0, 0);
}

static void replace3(void**) {
    DECLARE_CURSOR(1);
    row.append(b, f, 1);
    row.append(a, f, 1);
    row.append(a, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(0, 1, 1, 0, 0, 0);
}

static void replace4(void**) {
    DECLARE_CURSOR(1);
    row.append(a, f, 1);
    row.append(a, f, 1);
    row.append(b, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(1, 2, 1, 0, 0, 0);
}

static void replace5(void**) {
    DECLARE_CURSOR(0);
    row.append(a, f, 1);
    row.append(a, f, 1);
    row.append(a, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(0, 0, 1, 0, 0, 0);
}

static void replace6(void**) {
    DECLARE_CURSOR(0);
    row.append(a, f, 1);
    row.append(b, f, 1);
    row.append(a, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(0, 1, 1, 0, 0, 0);
}

static void replace7(void**) {
    DECLARE_CURSOR(2);
    row.append(a, f, 1);
    row.append(a, f, 1);
    row.append(a, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(2, 2, 1, 0, 0, 0);
}

static void replace8(void**) {
    DECLARE_CURSOR(2);
    row.append(a, f, 1);
    row.append(b, f, 1);
    row.append(a, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(1, 2, 1, 0, 0, 0);
}

static void replace9(void**) {
    DECLARE_CURSOR(1);
    row.append(a, f, 1);
    row.append(b, f, 1);
    row.append(a, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, a, f, 1, wl);

    }

static void replace10(void**) {
    DECLARE_CURSOR(1);
    row.append(b, f, 1);
    row.append(b, f, 1);
    row.append(b, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, a, f, 1, wl);

    ASSERT_RANGES(0, 0, 1, 0, 0, 0, 2, 2, 1, 0, 0, 0);
}

/*
 * replace 2-way tests
 */
static void replace11(void**) {
    DECLARE_CURSOR(1);
    row.append(c, f, 1);
    row.append(c, f, 1);
    row.append(b, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(0, 0, 2, 0, 0, 0, 1, 2, 1, 0, 0, 0);
}

static void replace12(void**) {
    DECLARE_CURSOR(0);
    row.append(c, f, 1);
    row.append(c, f, 1);
    row.append(b, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(0, 0, 1, 0, 0, 0, 1, 1, 2, 0, 0, 0, 2, 2, 1, 0, 0, 0);
}

static void replace13(void**) {
    DECLARE_CURSOR(1);
    row.append(c, f, 1);
    row.append(c, f, 1);
    row.append(c, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(0, 0, 2, 0, 0, 0, 1, 1, 1, 0, 0, 0, 2, 2, 2, 0, 0, 0);
}

static void replace14(void**) {
    DECLARE_CURSOR(1);
    row.append(c, f, 1);
    row.append(b, f, 1);
    row.append(c, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, c, f, 1, wl);

    ASSERT_RANGES(0, 2, 2, 0, 0, 0);
}

static void replace15(void**) {
    DECLARE_CURSOR(0);
    row.append(c, f, 1);
    row.append(c, f, 1);
    row.append(c, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(0, 0, 1, 0, 0, 0, 1, 2, 2, 0, 0, 0);
}

static void replace16(void**) {
    DECLARE_CURSOR(2);
    row.append(c, f, 1);
    row.append(c, f, 1);
    row.append(c, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, b, f, 1, wl);

    ASSERT_RANGES(0, 1, 2, 0, 0, 0, 2, 2, 1, 0, 0, 0);
}

/*
 * replace 3-way tests
 */
static void replace20(void**) {
    DECLARE_CURSOR(1);
    row.append(b, f, 1);
    row.append(c, f, 1);
    row.append(b, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, d, f, 1, wl);

    ASSERT_RANGES(0, 0, 1, 0, 0, 0, 1, 1, 3, 0, 0, 0, 2, 2, 1, 0, 0, 0);
}

static void replace21(void**) {
    DECLARE_CURSOR(1);
    row.append(c, f, 1);
    row.append(c, f, 1);
    row.append(b, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, d, f, 1, wl);

    ASSERT_RANGES(0, 0, 2, 0, 0, 0, 1, 1, 3, 0, 0, 0, 2, 2, 1, 0, 0, 0);
}

static void replace22(void**) {
    DECLARE_CURSOR(1);
    row.append(b, f, 1);
    row.append(c, f, 1);
    row.append(c, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, d, f, 1, wl);

    ASSERT_RANGES(0, 0, 1, 0, 0, 0, 1, 1, 3, 0, 0, 0, 2, 2, 2, 0, 0, 0);
}

static void replace23(void**) {
    DECLARE_CURSOR(1);
    row.append(b, f, 1);
    row.append(c, f, 1);
    row.append(d, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, d, f, 1, wl);

    ASSERT_RANGES(0, 0, 1, 0, 0, 0, 1, 2, 3, 0, 0, 0);
}

static void replace24(void**) {
    DECLARE_CURSOR(1);
    row.append(d, f, 1);
    row.append(c, f, 1);
    row.append(b, f, 1);

    row.updateCursor(cursor, wl);
    row.replace(cursor, d, f, 1, wl);

    ASSERT_RANGES(0, 1, 3, 0, 0, 0, 2, 2, 1, 0, 0, 0);
}

int main()
{
    REGISTER_UNIPLUGIN(uniplugin_termy_init);

    const CMUnitTest tests[] = {
        cmocka_unit_test(push1),
        cmocka_unit_test(push2),
        cmocka_unit_test(push3),
        cmocka_unit_test(push4),
        cmocka_unit_test(push5),
        cmocka_unit_test(push6),
        cmocka_unit_test(replace1),
        cmocka_unit_test(replace2),
        cmocka_unit_test(replace3),
        cmocka_unit_test(replace4),
        cmocka_unit_test(replace5),
        cmocka_unit_test(replace6),
        cmocka_unit_test(replace7),
        cmocka_unit_test(replace8),
        cmocka_unit_test(replace9),
        cmocka_unit_test(replace10),
        cmocka_unit_test(replace11),
        cmocka_unit_test(replace12),
        cmocka_unit_test(replace13),
        cmocka_unit_test(replace14),
        cmocka_unit_test(replace15),
        cmocka_unit_test(replace16),
        cmocka_unit_test(replace20),
        cmocka_unit_test(replace21),
        cmocka_unit_test(replace22),
        cmocka_unit_test(replace23),
        cmocka_unit_test(replace24),
    };

    b.flags = 1;
    c.flags = 2;
    d.flags = 3;

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
