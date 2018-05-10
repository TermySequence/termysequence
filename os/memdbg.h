// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#if MEMDEBUG
extern void memDebug();
#else
#define memDebug()
#endif
