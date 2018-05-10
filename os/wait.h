// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

extern int
osWaitForAny(int *status);

extern bool
osWaitForChild(int pid, int *status = nullptr);

extern bool
osExitedOnSignal(int status) __attribute__((const));

extern int
osExitStatus(int status) __attribute__((const));

extern int
osExitSignal(int status) __attribute__((const));

extern bool
osDumpedCore(int status) __attribute__((const));
