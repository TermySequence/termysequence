// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/stringmap.h"

extern void
osStatusInit(void **data);

extern void
osStatusTeardown(void *data);

extern void
osGetProcessAttributes(void *data, int pid, StringMap &current, StringMap &next);

extern StringMap
osGetProcessEnvironment(int pid);

extern StringMap
osGetLocalEnvironment();
