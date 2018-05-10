// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "util.h"

int
main(int argc, char **argv)
{
    setSignalHandler();
    tty_raw(STDIN_FILENO);

    while (1) {
        char buf[2048];
        ssize_t rc = read(STDIN_FILENO, buf, sizeof(buf));
        if (rc <= 0)
            break;
        usleep(50000);
    }

    return 0;
}
