// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#define TR_EXIT1 TL("server", "Closed", "exit1")
#define TR_EXIT2 TL("server", "Server exiting on signal", "exit2")
#define TR_EXIT3 TL("server", "Forwarder exiting on signal", "exit3")
#define TR_EXIT4 TL("server", "Server reported error", "exit4")
#define TR_EXIT5 TL("server", "Forwarding error", "exit5")
#define TR_EXIT6 TL("server", "Protocol version mismatch", "exit6")
#define TR_EXIT7 TL("server", "Protocol parse error", "exit7")
#define TR_EXIT8 TL("server", "Duplicate connection", "exit8")
#define TR_EXIT9 TL("server", "Lost connection to server", "exit9")
#define TR_EXIT10 TL("server", "Connection limit exceeded", "exit10")
#define TR_EXIT11 TL("server", "Connection timed out", "exit11")

// 0-2 are generic task errors
#define TR_CONNERR3 TL("server", "Write error", "connerr3")
#define TR_CONNERR4 TL("server", "Read error during remote handshake", "connerr4")
#define TR_CONNERR5 TL("server", "Failed to connect to remote server", "connerr5")
#define TR_CONNERR6 TL("server", "Remote handshake failed (code %1)", "connerr6")
#define TR_CONNERR7 TL("server", "Read limit exceeded during handshake", "connerr7")
#define TR_CONNERR8 TL("server", "Read error during local handshake", "connerr8")
#define TR_CONNERR9 TL("server", "Failed to connect to local server", "connerr9")
#define TR_CONNERR10 TL("server", "Local handshake failed (code %1)", "connerr10")
#define TR_CONNERR11 TL("server", "Transfer of file descriptors failed", "connerr11")
// 12 is protocol rejection with an exit status (see above)
#define TR_CONNERR13 TL("server", "Received bad protocol number %1", "connerr13")
#define TR_CONNERR14 TL("server", "Invalid local response (code %1)", "connerr14")
#define TR_CONNERR15 TL("server", "Failed to read connection id", "connerr15")
