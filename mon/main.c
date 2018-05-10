// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "monitor.h"

#include <stdio.h>

static void __attribute__((noreturn))
usage(void)
{
    fputs("Usage: " MONITOR_NAME " [--initial|--monitor]\n"
          "\t--initial: Print starting attributes and exit\n"
          "\t--monitor: Monitor for attribute changes (default)\n",
          stderr);

    exit(EXITCODE_ARGPARSE);
}

static int
parseArgs(int argc, char **argv)
{
    if (argc == 1)
        return 0;
    else if (argc > 2)
        usage();
    else if (strcmp(argv[1], "--initial") == 0)
        return 1;
    else if (strcmp(argv[1], "--monitor") != 0)
        usage();

    return 0;
}

int
main(int argc, char **argv)
{
    int initial = parseArgs(argc, argv);

    if (initial)
        return monitorInitial();
    else
        return monitorMonitor();
}
