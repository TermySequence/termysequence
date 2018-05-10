// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

extern void
osCreateRuntimeDir(const char *pathspec, char *pathret);

extern void
osCreateSocketPath(char *pathinout);

extern void
osCreatePidFile(char *pathinout);

extern int
osServerConnect(const char *pathspec);

extern int
osAppConnect();

extern void
osRelativeToHome(std::string &path);

extern bool
osConfigPath(const char *appname, const char *filename, std::string &result);

extern int
osCreateMountPath(const std::string &id, std::string &result);

extern int
osCreateNamedPipe(bool ro, unsigned mode, std::string &result);

extern bool
osFindBin(const char *name);
