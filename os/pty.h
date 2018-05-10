// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

extern void
osSetTerminal(int fd);

extern void
osResizeTerminal(int fd, unsigned short width, unsigned short height);

extern bool
osGetTerminalLockable(int fd);

extern void
osMakeRawTerminal(int fd, char **savedret = nullptr);

extern void
osMakeSignalTerminal(int fd, char **savedret);

extern void
osMakePasswordTerminal(int fd, char **savedret);

extern void
osRestoreTerminal(int fd, char *saved);

extern bool
osGetEmulatorAttribute(const char *name, std::string &value, bool *found = nullptr);

extern bool
osSetEmulatorAttribute(const char *name, const std::string &value);

extern bool
osClearEmulatorAttribute(const char *name);

extern bool
osGetTerminalAttributes(int fd, uint32_t *termiosFlags, char *termiosChars);
