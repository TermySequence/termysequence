// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/stringmap.h"
#include "lib/uuid.h"

#include <vector>

extern bool
osLoadFile(const char *path, StringMap &map);

extern void
osAttributes(StringMap &map, std::vector<int> &pids, bool isServer);

extern bool
osAttributesAsync(StringMap &map, int *fdret, int *pidret, int *state);

extern void
osIdentity(Tsq::Uuid &result, std::vector<int> &pids);

extern void
osFallbackIdentity(Tsq::Uuid &result);

extern bool
osRestrictedMonitorAttribute(const std::string &key);

extern bool
osRestrictedServerAttribute(const std::string &key);

extern bool
osRestrictedTermAttribute(const std::string &key, bool isOwner);

extern void
osPrintVersionAndExit(const char *name) __attribute__((noreturn));
