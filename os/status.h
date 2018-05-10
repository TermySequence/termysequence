// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <unordered_map>

extern void
osStatusInit(int **data);

extern void
osStatusTeardown(int *data);

extern void
osGetProcessAttributes(int *data, int pid,
                       std::unordered_map<std::string,std::string> &current,
                       std::unordered_map<std::string,std::string> &next);
