// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "args.h"
#include "conn.h"
#include "app/attrbase.h"
#include "app/protocol.h"
#include "os/signal.h"
#include "os/time.h"
#include "os/dir.h"
#include "os/conn.h"
#include "lib/exception.h"
#include "lib/wire.h"

#include <QTranslator>
#include <QLocale>
#include <cstdio>
#include <unistd.h>

#define TR_ERROR1 TL("cli", "Protocol version mismatch")

#define EXITCODE_ARGPARSE       1
#define EXITCODE_SIGNAL         2
#define EXITCODE_CONNECTERR     3

#define XLATE_DIR L(PREFIX "/share/" APP_NAME "/i18n")
const QString g_mtstr;

static int
doConnect()
{
    int fd = osAppConnect();
    if (fd == -1)
        throw Tsq::ErrnoException("connect", errno);

    // read the version number
    char buf[4];
    size_t got = 0;

    while (got < 4) {
        ssize_t rc = read(fd, buf + got, 4 - got);
        if (rc <= 0)
            throw Tsq::ErrnoException("read", rc ? errno : ECONNABORTED);
        got += rc;
    }

    Tsq::ProtocolUnmarshaler unm(buf, 4);
    if (unm.parseNumber() != TSQT_PROTOCOL_VERSION)
        throw Tsq::TsqException("%s", pr(TR_ERROR1));

    return fd;
}

int
main(int argc, char **argv)
{
    osInitMonotime();
    osSetupClientSignalHandlers();

    QCoreApplication app(argc, argv);
    QTranslator translator;
    if (translator.load(QLocale(), g_mtstr, g_mtstr, XLATE_DIR, A(".qm")))
        app.installTranslator(&translator);

    ArgParser args;

    try {
        args.parse(argc, argv);
    } catch (const std::exception &e) {
        fprintf(stderr, "%s\n", e.what());
        return EXITCODE_ARGPARSE;
    }

    try {
        int fd = doConnect();
        CliConnection conn(fd, &args);
        while (conn.run());
    } catch (const std::exception &e) {
        fprintf(stderr, "%s\n", e.what());
        return EXITCODE_CONNECTERR;
    }

    return 0;
}

extern "C" void
deathHandler(int signal)
{
    _exit(EXITCODE_SIGNAL);
}

extern "C" void
reloadHandler(int signal)
{
}
