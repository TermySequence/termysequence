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

extern const char *
osGetStdlibName(char *buf16);

extern const char *
osGetStdlibVersion(char *buf16);

#ifdef __cplusplus
}
#endif
