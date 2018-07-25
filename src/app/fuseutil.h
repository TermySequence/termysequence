// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#if USE_FUSE3

#define FUSE_USE_VERSION 31
#include <fuse3/fuse_lowlevel.h>
#define FUSE_CAP_BIG_WRITES 0
struct fuse_chan;

#elif USE_FUSE2

#define FUSE_USE_VERSION 26
#include <fuse/fuse_lowlevel.h>
#define FUSE_CAP_ASYNC_DIO 0
#define FUSE_CAP_HANDLE_KILLPRIV 0
#define FUSE_CAP_PARALLEL_DIROPS 0
#define FUSE_SET_ATTR_CTIME 0

#else

struct fuse_session;
struct fuse_chan;
struct fuse_buf {};
struct fuse_args {};
typedef void *fuse_req_t;

#endif
