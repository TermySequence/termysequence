// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#if USE_SYSTEMD

class ServerInstance;
class TermManager;

extern bool needSystemdSetup(ServerInstance *persistent, ServerInstance *transient);
extern void showSystemdSetup(TermManager *manager);

#else // USE_SYSTEMD

#define needSystemdSetup(x, y) false
#define showSystemdSetup(x)

#endif // USE_SYSTEMD
