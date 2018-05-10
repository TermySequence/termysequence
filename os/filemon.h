// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#ifdef __linux__
#include <sys/inotify.h>
#else
struct inotify_event {
    int wd;
    unsigned mask;
    unsigned len;
    char name[];
};

#define IN_CLOEXEC 0
#define IN_NONBLOCK 0

#define inotify_init1(x) (-1)
#define inotify_add_watch(x, y, z) (-1)

#define IN_MODIFY 0
#define IN_ATTRIB 0
#define IN_MOVED_FROM 0
#define IN_MOVE 0
#define IN_CREATE 0
#define IN_DELETE 0
#define IN_DELETE_SELF 0
#define IN_MOVE_SELF 0
#define IN_IGNORED 0
#define IN_ONLYDIR 0
#define IN_EXCL_UNLINK 0
#endif
