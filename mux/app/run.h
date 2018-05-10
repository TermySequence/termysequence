// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

extern void
runConnect();

extern void
runListen();

extern int
runServer();

extern void
runForwarder(int fd) __attribute__((noreturn));

extern int
runConnector();

extern int
runQuery();
