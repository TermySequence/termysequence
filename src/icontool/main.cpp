// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "settings.h"
#include "setup.h"
#include "app.h"

#include <QApplication>
#include <QDir>
#include <cstdio>

const QString g_mtstr;

[[noreturn]] static void
usage()
{
    fputs("Usage: " ICONTOOL_NAME " [update|install]\n", stderr);
    exit(1);
}

static bool
directoryExists(const QString &path)
{
    QFileInfo fi(path);
    return fi.isDir() && fi.isReadable();
}

static bool
fileExists(const QString &path)
{
    QFileInfo fi(path);
    return fi.isFile() && fi.isReadable();
}

static bool
validateSettings()
{
    if (!directoryExists(g_settings->repo()))
        return false;
    if (!fileExists(g_settings->workfile()))
        return false;

    for (const auto &i: g_settings->sources()) {
        if (!directoryExists(i.path))
            return false;
        if (!directoryExists(i.path + A("/16x16")))
            return false;
    }
    return true;
}

static void
loadSettings(QCoreApplication *app, bool interactive)
{
    g_settings = new IconSettings(app);

    while (!validateSettings()) {
        if (!interactive || QDialog::Accepted != (new SetupDialog)->exec()) {
            fputs("Error: settings do not validate\n", stderr);
            exit(2);
        }
    }
}

int
main(int argc, char **argv)
{
    unsigned command = 0;
    QCoreApplication *app;

    switch (argc) {
    case 1:
        app = new QApplication(argc, argv);
        break;
    case 2:
        app = new QCoreApplication(argc, argv);
        if (!strcmp(argv[1], "update")) {
            command = 1;
            break;
        }
        if (!strcmp(argv[1], "install")) {
            command = 2;
            break;
        }
        // fallthru
    default:
        usage();
    }

    app->setOrganizationName(ORG_NAME);
    app->setOrganizationDomain(ORG_DOMAIN);
    app->setApplicationName(ICONTOOL_NAME);
    app->setApplicationVersion(PROJECT_VERSION);

    loadSettings(app, command == 0);

    auto *iapp = new IconApplication(app, command);
    return iapp->exec();
}
