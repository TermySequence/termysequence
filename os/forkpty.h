// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "process.h"
#include "pty.h"

extern int
osForkPty(const PtyParams &params, int *fdret, char *pathret);
