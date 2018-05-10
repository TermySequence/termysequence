// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern int
osGetCpuCores();

extern int
osGetLoadAverage(char *buf, unsigned buflen);

#ifdef __cplusplus
}
#endif
