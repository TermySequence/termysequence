// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

static int s_fd = -1;

static void
signalHandler(int signal)
{
    if (s_fd != -1) {
        char buf[64] = "Received A\n";
        buf[9] += signal;
        write(s_fd, buf, 11);
        fsync(s_fd);
    }
}

static void
usage()
{
    fputs("Usage: recalcitrant [-f logfile]\n", stderr);
    exit(1);
}

int
main(int argc, char **argv)
{
    if (argc != 1 && (argc != 3 || strcmp(argv[1], "-f")))
        usage();

    if (argc == 3) {
        s_fd = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
        if (s_fd < 0) {
            fprintf(stderr, "%s: %m\n", argv[2]);
            return 1;
        }
    }

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signalHandler;
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    for (;;) {
        sleep(1);
    }

    return 0;
}
