// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#if USE_FUSE
#define FUSE_USE_VERSION 31
#include <fuse3/fuse_lowlevel.h>
#else
struct fuse_session;
struct fuse_buf {};
struct fuse_args {};
typedef void *fuse_req_t;
#endif
