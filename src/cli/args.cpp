// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/protocol.h"
#include "args.h"
#include "os/attr.h"
#include "lib/exception.h"

#include <cstdio>
#include <unistd.h>

#define TR_PARSE1 TL("cli", "No command specified")
#define TR_PARSE2 TL("cli", "Unrecognized command")
#define TR_PARSE3 TL("cli", "Command requires an argument")

void
ArgParser::handleHelp()
{
    printf(PIPE_NAME " COMMAND ARG\n\n"
           "to USER@HOST     Pipe stdin to a named pipe on a remote server\n"
           "from USER@HOST   Pipe stdout from a named pipe on a remote server\n"
           "list-servers     List currently connected servers\n"
           "invoke ACTION... Run one or more application actions\n\n"
           "--help       Show this help\n"
           "--man        Launch man page\n"
           "--version    Show version information\n");
    exit(0);
}

void
ArgParser::parse(int argc, char **argv)
{
    if (argc == 1)
        throw Tsq::TsqException("%s", pr(TR_PARSE1));

    if (!strcmp(argv[1], "invoke")) {
        m_cmd = TSQT_RUN_ACTION;
        if (argc < 3)
            goto arg;
    }
    else if (!strcmp(argv[1], "to")) {
        m_cmd = TSQT_UPLOAD_PIPE;
        if (argc != 3)
            goto arg;
    }
    else if (!strcmp(argv[1], "from")) {
        m_cmd = TSQT_DOWNLOAD_PIPE;
        if (argc != 3)
            goto arg;
    }
    else if (!strcmp(argv[1], "list-servers")) {
        m_cmd = TSQT_LIST_SERVERS;
    }
    else if (!strcmp(argv[1], "--help"))
        handleHelp();
    else if (!strcmp(argv[1], "--man"))
        execlp("man", "man", PIPE_NAME, NULL), _exit(127);
    else if (!strcmp(argv[1], "--version"))
        osPrintVersionAndExit(PIPE_NAME);
    else
        throw Tsq::TsqException("%s", pr(TR_PARSE2 + A(": ") + argv[1]));

    m_args = argv + 2;
    m_argc = argc - 2;
    return;
arg:
    throw Tsq::TsqException("%s", pr(TR_PARSE3 + A(": ") + argv[1]));
}
