// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include <sys/types.h>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <execinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <new>

#define MEMDBG_PRELOAD "memdbg.so"
#define MEMDBG_LEN 9
#define MEMDBG_OUT "/tmp/memcrash.txt"

extern "C" void memCallback(int type, size_t size)
{
    int fd = open(MEMDBG_OUT, O_CREAT|O_WRONLY|O_APPEND|O_CLOEXEC, 0644);
    if (fd >= 0) {
        write(fd, "Crash report:   ", 16);
        write(fd, &type, sizeof(type));
        write(fd, &size, sizeof(size));
        write(fd, "\n", 1);

        void *buf[64];
        int rc = backtrace(buf, 64);
        backtrace_symbols_fd(buf, rc, fd);
    }

    abort();
}

static void memHandler()
{
    memCallback(0, 0);
}

void memDebug()
{
    // Check if the LD_PRELOAD shim is present
    const char *shim = getenv("LD_PRELOAD");
    int len = shim ? strlen(shim) : 0;

    if (len >= MEMDBG_LEN && !strcmp(shim + len - MEMDBG_LEN, MEMDBG_PRELOAD))
    {
        unsetenv("LD_PRELOAD");

        // Give our callback to the LD_PRELOAD shim
        intptr_t trigger = -1;
        free((void *)trigger);
        free((void *)memCallback);
    }

    // Set new handler
    std::set_new_handler(memHandler);
}
