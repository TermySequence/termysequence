// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void osInitMonotime();

extern int64_t osMonotime();

extern int64_t osWalltime();

extern int64_t osBasetime(int64_t *baseret);

extern int32_t osModtime(int64_t base);

extern int64_t osSigtime();

#ifdef __cplusplus
}
#endif
