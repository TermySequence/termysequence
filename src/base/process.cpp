// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "process.h"
#include "lib/enums.h"
#include "lib/wire.h"

TermProcess::TermProcess() :
    status(Tsq::TermIdle),
    outcome(Tsq::TermRunning),
    commandName(g_str_unknown),
    commandArgv(g_str_unknown)
{
}

void
TermProcess::setAttribute(const QString &key, const QString &value)
{
    if (key == g_attr_PROC_TERMIOS) {
        QByteArray code = QByteArray::fromBase64(value.toUtf8());
        if (code.size() == (12 + Tsq::TermiosNChars)) {
            const char *ptr = code.data();
            const uint32_t *flags = reinterpret_cast<const uint32_t*>(ptr);

            termiosInputFlags = flags[0];
            termiosOutputFlags = flags[1];
            termiosLocalFlags = flags[2];
            termiosRaw = !(termiosLocalFlags & Tsq::TermiosISIG);

            #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            termiosInputFlags = __builtin_bswap32(termiosInputFlags);
            termiosOutputFlags = __builtin_bswap32(termiosOutputFlags);
            termiosLocalFlags = __builtin_bswap32(termiosLocalFlags);
            #endif

            memcpy(termiosChars, ptr + 12, Tsq::TermiosNChars);
        }
    }
    else if (key == g_attr_PROC_PID) {
        pid = value.toInt();
    }
    else if (key == g_attr_PROC_CWD) {
        workingDir = value;
    }
    else if (key == g_attr_PROC_COMM) {
        commandName = value;
    }
    else if (key == g_attr_PROC_ARGV) {
        commandArgv = value.split('\x1f');
    }
    else if (key == g_attr_PROC_STATUS) {
        status = value.toInt();
    }
    else if (key == g_attr_PROC_OUTCOME) {
        outcome = value.toInt();
    }
    else if (key == g_attr_PROC_OUTCOMESTR) {
        outcomeStr = value;
    }
    else if (key == g_attr_PROC_EXITCODE) {
        exitStatus = value.toInt();
    }
}

void
TermProcess::clearAttribute(const QString &key)
{
    if (key == g_attr_PROC_TERMIOS) {
        termiosInputFlags = 0;
        termiosOutputFlags = 0;
        termiosLocalFlags = 0;
        memset(termiosChars, 0, Tsq::TermiosNChars);
        termiosRaw = false;
    }
    else if (key == g_attr_PROC_PID) {
        pid = -1;
    }
    else if (key == g_attr_PROC_CWD) {
        workingDir.clear();
    }
    else if (key == g_attr_PROC_COMM) {
        commandName.clear();
    }
    else if (key == g_attr_PROC_ARGV) {
        commandArgv.clear();
    }
    else if (key == g_attr_PROC_STATUS) {
        status = Tsq::TermIdle;
    }
    else if (key == g_attr_PROC_OUTCOME) {
        outcome = Tsq::TermRunning;
    }
    else if (key == g_attr_PROC_OUTCOMESTR) {
        outcomeStr.clear();
    }
    else if (key == g_attr_PROC_EXITCODE) {
        exitStatus = -1;
    }
}
