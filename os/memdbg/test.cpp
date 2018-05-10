// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include <sys/types.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <execinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <new>

extern "C" void memCallback(int type, size_t size)
{
    write(STDERR_FILENO, "Crash report:   ", 16);
    write(STDERR_FILENO, &type, sizeof(type));
    write(STDERR_FILENO, &size, sizeof(size));
    write(STDERR_FILENO, "\n", 1);

    void *buf[64];
    int rc = backtrace(buf, 64);
    backtrace_symbols_fd(buf, rc, STDERR_FILENO);

    abort();
}

static void memHandler()
{
    memCallback(0, 0);
}

static void usage()
{
    fputs("Usage: LD_PRELOAD=memdbg.so memdbgtest [m|n] <alloc_order>\n", stderr);
    _exit(1);
}

static void memDebug()
{
    // Test LD_PRELOAD
    const char *shim = getenv("LD_PRELOAD");
    int len = shim ? strlen(shim) : 0;
    if (len < 9 || strcmp(shim + len - 9, "memdbg.so"))
        usage();

    // Give our malloc-failure callback to the LD_PRELOAD shim
    intptr_t trigger = -1;
    free((void *)trigger);
    free((void *)&memCallback);

    // Set new handler
    std::set_new_handler(memHandler);
}

int main(int argc, char **argv)
{
    memDebug();

    if (argc != 3)
        usage();

    unsigned power = atoi(argv[2]);
    char *poison;

    if (argv[1][0] == 'm') {
        poison = (char *)std::malloc(1l << power);
        printf("Malloc failure: %p\n", poison);
    }
    else {
        poison = new char[1l << power];
        printf("New failure: %p\n", poison);
    }

    return 0;
}
