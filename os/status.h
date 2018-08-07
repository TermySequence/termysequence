// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/stringmap.h"

extern void
osStatusInit(int **data);

extern void
osStatusTeardown(int *data);

extern void
osGetProcessAttributes(int *data, int pid, StringMap &current, StringMap &next);

extern StringMap
osGetProcessEnvironment(int pid);

extern StringMap
osGetLocalEnvironment();
