// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/flags.h"

#include <QStringList>

struct TermProcess
{
    int status;
    int outcome;
    // int ppid;
    int pid = -1;
    // int uid;
    // int gid;
    int exitStatus = -1;
    // QString user;
    // QString group;
    // QString seclabel;
    QString commandName;
    QString commandPath;
    QStringList commandArgv;
    QString workingDir;
    QString outcomeStr;
    Tsq::TermiosFlags termiosInputFlags = 0;
    Tsq::TermiosFlags termiosOutputFlags = 0;
    Tsq::TermiosFlags termiosLocalFlags = 0;
    char termiosChars[Tsq::TermiosNChars]{};
    bool termiosRaw = false;

public:
    TermProcess();

    void setAttribute(const QString &key, const QString &value);
    void clearAttribute(const QString &key);
};
