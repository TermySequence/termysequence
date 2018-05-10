// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

extern "C" void deathHandler(int signal);
extern "C" void reloadHandler(int signal);

extern void
osSetupServerSignalHandlers();

extern void
osSetupClientSignalHandlers();

extern void
osIgnoreBackgroundSignals();

extern void
osKillProcess(int pgrp, int signal = 0);
