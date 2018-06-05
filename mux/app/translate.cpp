// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "args.h"
#include "os/attr.h"
#include "os/logging.h"
#include "lib/protocol.h"
#include "lib/enums.h"
#include "lib/exitcode.h"
#include "lib/trstr.h"

#include <cstdio>

inline ArgParser::Lock::Lock(ArgParser *t): m_t(t)
{
    pthread_mutex_lock(&m_t->m_lock);
}

inline ArgParser::Lock::~Lock()
{
    pthread_mutex_unlock(&m_t->m_lock);
}

const char *
Translator::get(const std::string &key, const char *defval) const
{
    auto i = find(key);
    return i != cend() ? i->second.c_str() : defval;
}

ArgParser::~ArgParser()
{
    for (auto &&i: m_translators)
        delete i.second;

    pthread_mutex_destroy(&m_lock);
}

Translator *
ArgParser::makeTranslator(std::string lang)
{
    if (lang.empty() || lang.find('/') != std::string::npos)
        return nullptr;

    size_t idx = lang.find('.');
    if (idx != std::string::npos)
        lang.erase(idx);

    Translator scratch;
    scratch.path = DATADIR "/" SERVER_NAME "/i18n/";
    scratch.path += lang + ".txt";

    if (osLoadFile(scratch.path.c_str(), scratch))
        return new Translator(std::move(scratch));

    idx = lang.find('_');
    if (idx != std::string::npos && idx > 0) {
        lang.erase(idx);
        return makeTranslator(lang);
    }

    return nullptr;
}

const Translator *
ArgParser::getTranslator(const std::string &lang)
{
    Lock lock(this);

    auto i = m_translators.find(lang);
    if (i != m_translators.end())
        return i->second;

    auto *translator = makeTranslator(lang);
    if (translator) {
        LOGDBG("Translator: Loaded %s for %s\n", translator->path.c_str(), lang.c_str());
        m_translators.emplace(lang, translator);
        return translator;
    }

    LOGDBG("Translator: Using defaults for %s\n", lang.c_str());
    return m_translator;
}

void
ArgParser::arg(std::string &str, const char *fmt, int arg1, int arg2)
{
    str = fmt;
    size_t idx = str.find("%1", 0, 2);
    if (idx != std::string::npos)
        str.replace(idx, 2, std::to_string(arg1));
    idx = str.find("%2", 0, 2);
    if (idx != std::string::npos)
        str.replace(idx, 2, std::to_string(arg2));
}

void
ArgParser::arg(std::string &str, const char *fmt, const char *arg)
{
    str = fmt;
    size_t idx = str.find("%1", 0, 2);
    if (idx != std::string::npos)
        str.replace(idx, 2, arg);
}

int
ArgParser::printExitStatus(int code)
{
    const char *message;

    switch (code) {
    case TSQ_STATUS_NORMAL:
        return 0;
    case TSQ_STATUS_CLOSED:
        message = TR_EXIT1;
        break;
    case TSQ_STATUS_SERVER_SHUTDOWN:
        message = TR_EXIT2;
        break;
    case TSQ_STATUS_FORWARDER_SHUTDOWN:
        message = TR_EXIT3;
        break;
    case TSQ_STATUS_SERVER_ERROR:
        message = TR_EXIT4;
        break;
    case TSQ_STATUS_FORWARDER_ERROR:
        message = TR_EXIT5;
        break;
    case TSQ_STATUS_PROTOCOL_MISMATCH:
        message = TR_EXIT6;
        break;
    case TSQ_STATUS_PROTOCOL_ERROR:
        message = TR_EXIT7;
        break;
    case TSQ_STATUS_DUPLICATE_CONN:
        message = TR_EXIT8;
        break;
    case TSQ_STATUS_LOST_CONN:
        message = TR_EXIT9;
        break;
    case TSQ_STATUS_CONN_LIMIT_REACHED:
        message = TR_EXIT10;
        break;
    case TSQ_STATUS_IDLE_TIMEOUT:
        message = TR_EXIT11;
        break;
    default:
        goto out;
    }

    fprintf(stderr, "%s\n", message);
out:
    return EXITCODE_STATUSBASE + code;
}

int
ArgParser::printConnectError(int errtype, int errnum, int errsave)
{
    int status = EXITCODE_CONNECTERR;
    std::string message;

    switch (errtype) {
    case Tsq::ConnectTaskErrorWriteFailed:
        message = TR_CONNERR3;
        break;
    case Tsq::ConnectTaskErrorRemoteReadFailed:
        message = TR_CONNERR4;
        break;
    case Tsq::ConnectTaskErrorRemoteConnectFailed:
        message = TR_CONNERR5;
        break;
    case Tsq::ConnectTaskErrorRemoteHandshakeFailed:
        arg(message, TR_CONNERR6, errnum);
        break;
    case Tsq::ConnectTaskErrorRemoteLimitExceeded:
        message = TR_CONNERR7;
        break;
    case Tsq::ConnectTaskErrorLocalReadFailed:
        message = TR_CONNERR8;
        break;
    case Tsq::ConnectTaskErrorLocalConnectFailed:
        message = TR_CONNERR9;
        break;
    case Tsq::ConnectTaskErrorLocalHandshakeFailed:
        arg(message, TR_CONNERR10, errnum);
        break;
    case Tsq::ConnectTaskErrorLocalTransferFailed:
        message = TR_CONNERR11;
        break;
    case Tsq::ConnectTaskErrorLocalRejection:
        return printExitStatus(errnum);
    case Tsq::ConnectTaskErrorLocalBadProtocol:
        arg(message, TR_CONNERR13, errnum);
        break;
    case Tsq::ConnectTaskErrorLocalBadResponse:
        arg(message, TR_CONNERR14, errnum);
        break;
    default:
        message = TR_CONNERR15;
    }

    if (errsave)
        fprintf(stderr, "%s: %s\n", message.c_str(), strerror(errsave));
    else
        fprintf(stderr, "%s\n", message.c_str());

    return status;
}
