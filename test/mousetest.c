// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "util.h"

static void
term_output(const char *str)
{
    write(STDOUT_FILENO, str, strlen(str));
}

int
main(int argc, char **argv)
{
    char c;
    FILE *f;
    int rc;
    char filename[64];
    unsigned mode, enc;

    if (argc != 4) {
        fputs("Usage: mousetest <mousemode> <encmode> <suffix>\n", stderr);
        return 2;
    }
    mode = atoi(argv[1]);
    enc = atoi(argv[2]);
    snprintf(filename, sizeof(filename), "/tmp/%d-%d-%s.out", mode, enc, argv[3]);
    f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
        return 3;
    }

    setSignalHandler();
    tty_raw(STDIN_FILENO);

    // Enter appropriate mouse mode
    term_output("\x1b[?1049h");
    term_output("\x1b[?");
    term_output(argv[1]);
    term_output("h");
    term_output("\x1b[?");
    term_output(argv[2]);
    term_output("h");

    // Draw click points
    term_output("\x1b[?2J");
    term_output("\x1b[H");
    term_output("\x1b[7m ");
    term_output("\x1b[m    ");
    term_output("\x1b[7m ");
    term_output("\x1b[m ");

    for (;;) {
        rc = read(STDIN_FILENO, &c, 1);
        if (rc <= 0)
            break;

        if (isprint((unsigned char)c))
            fprintf(f, "%c ", c);
        else
            fprintf(f, "(%d) ", c);

        if (c == 27)
            fputc('\n', f);

        fflush(f);
    }

    term_output("\x1b[?1049l");
    tty_unraw(STDIN_FILENO);
    return rc ? 1 : 0;
}
