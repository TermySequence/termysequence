// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include <cstdio>
#include <unistd.h>

#include <utf8.h>

static void
usage()
{
    fputs("Usage: unidraw <startcode> <nrows>\n", stderr);
    exit(1);
}

static void
die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(2);
}

static void
move_to_column(unsigned column)
{
    printf("\x1b[%d`", (column * 3) + 11);
}

static void
print_row_number(unsigned start)
{
    printf("\x1b[`U+%05Xx", start >> 4);
}

static void
print_codepoint(unsigned codepoint)
{
    char buf[8];
    size_t n = utf8::unchecked::append(codepoint, buf) - buf;
    for (size_t i = 0; i < n; ++i)
        putchar(buf[i]);
}

int
main(int argc, char **argv)
{
    if (argc != 3)
        usage();

    unsigned nrows = atoi(argv[2]);

    if (nrows > 16)
        die("Too many rows (up to 16 supported)");

    unsigned start = strtoul(argv[1], NULL, 16);
    start &= 0x10fff0;

    // Print header
    for (unsigned i = 0; i < 16; ++i) {
        move_to_column(i);
        printf("%01X", i);
    }
    puts("\n");

    // Print rows
    for (unsigned i = 0; i < nrows; ++i) {
        if (start > 0x10ffff)
            die("Highest code point reached");

        print_row_number(start);

        for (unsigned j = 0; j < 16; ++j) {
            move_to_column(j);
            print_codepoint(start++);
        }

        puts("\n");
    }

    return 0;
}
