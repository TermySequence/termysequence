// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "cellunit.h"

static void frontSubstr(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    auto str = row.substr(0, 1, wl);

    assert_string_equal(str.c_str(), "a" CMB);
}

static void backSubstr(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    auto str1 = row.substr(8, wl);
    auto str2 = row.substr(8, 9, wl);

    assert_string_equal(str1.c_str(), "b");
    assert_string_equal(str2.c_str(), "b");
}

static void midSubstr(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    auto str = row.substr(3, 6, wl);

    assert_string_equal(str.c_str(), "b" DW CMB CMB CMB "a" CMB);
}

static void fullSubstr(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    auto str1 = row.substr(0, wl);
    auto str2 = row.substr(0, 9, wl);

    assert_string_equal(str1.c_str(), FULL_STR);
    assert_string_equal(str2.c_str(), FULL_STR);
}

static void zeroWidthFrontSubstr(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    auto str = row.substr(0, 0, wl);

    assert_string_equal(str.c_str(), "");
}

static void zeroWidthBackSubstr(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    auto str1 = row.substr(9, wl);
    auto str2 = row.substr(9, 9, wl);

    assert_string_equal(str1.c_str(), "");
    assert_string_equal(str2.c_str(), "");
}

static void zeroWidthMidSubstr(void**)
{
    DECLARE_VARS;
    LOAD_STR(FULL_STR, 9, 14);

    auto str = row.substr(5, 5, wl);

    assert_string_equal(str.c_str(), "");
}

static void zeroWidthFullSubstr(void**)
{
    DECLARE_VARS;

    auto str1 = row.substr(0, wl);
    auto str2 = row.substr(0, 0, wl);

    assert_string_equal(str1.c_str(), "");
    assert_string_equal(str2.c_str(), "");
}

int main()
{
    const CMUnitTest tests[] = {
        cmocka_unit_test(frontSubstr),
        cmocka_unit_test(backSubstr),
        cmocka_unit_test(midSubstr),
        cmocka_unit_test(fullSubstr),
        cmocka_unit_test(zeroWidthFrontSubstr),
        cmocka_unit_test(zeroWidthBackSubstr),
        cmocka_unit_test(zeroWidthMidSubstr),
        cmocka_unit_test(zeroWidthFullSubstr),
    };

    return cmocka_run_group_tests(tests, nullptr, nullptr);
}
