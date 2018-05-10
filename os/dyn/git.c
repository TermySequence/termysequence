// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "os/logging.h"
#define LIBGIT2_NO_IFACE
#include "os/git.h"

#include <stdio.h>
#include <dlfcn.h>

#ifdef __linux__
#define GIT2_SONAME "libgit2.so.%u"
#else /* Mac OS X */
#define GIT2_SONAME "libgit2.%u.dylib"
#endif

DECLSYM(git_branch_name);
DECLSYM(git_branch_upstream);
DECLSYM(git_buf_free);
DECLSYM(git_describe_commit);
DECLSYM(git_describe_format);
DECLSYM(git_describe_result_free);
DECLSYM(git_graph_ahead_behind);
DECLSYM(git_object_free);
DECLSYM(git_reference_free);
DECLSYM(git_reference_name);
DECLSYM(git_reference_name_to_id);
DECLSYM(git_reference_peel);
DECLSYM(git_reference_shorthand);
DECLSYM(git_repository_free);
DECLSYM(git_repository_head);
DECLSYM(git_repository_head_detached);
DECLSYM(git_repository_open_ext);
DECLSYM(git_repository_workdir);
DECLSYM(git_status_file);

#define LOADSYM(x) ok = ok && (p_ ## x = dlsym(h, #x))

int
osLoadLibgit2()
{
    int ok = 1;
    char buf[64];
    void *h;

    snprintf(buf, sizeof(buf), GIT2_SONAME, LIBGIT2_SOVERSION);
    dlerror();
    if (!(h = dlopen(buf, RTLD_LAZY)))
        goto fail;

    DECLSYM(git_libgit2_init);
    LOADSYM(git_libgit2_init);
    LOADSYM(git_branch_name);
    LOADSYM(git_branch_upstream);
    LOADSYM(git_buf_free);
    LOADSYM(git_describe_commit);
    LOADSYM(git_describe_format);
    LOADSYM(git_describe_result_free);
    LOADSYM(git_graph_ahead_behind);
    LOADSYM(git_object_free);
    LOADSYM(git_reference_free);
    LOADSYM(git_reference_name);
    LOADSYM(git_reference_name_to_id);
    LOADSYM(git_reference_peel);
    LOADSYM(git_reference_shorthand);
    LOADSYM(git_repository_free);
    LOADSYM(git_repository_head);
    LOADSYM(git_repository_head_detached);
    LOADSYM(git_repository_open_ext);
    LOADSYM(git_repository_workdir);
    LOADSYM(git_status_file);

    if (ok) {
        (*p_git_libgit2_init)();
        LOGDBG("Loaded %s\n", buf);
        return 1;
    }
fail:
    LOGDBG("Failed to load %s: %s\n", buf, dlerror());
    return 0;
}
