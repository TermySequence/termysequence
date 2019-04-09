// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "args.h"
#include "base/exception.h"
#include "os/attr.h"
#include "os/locale.h"
#include "config.h"

#include <cstdio>
#include <unistd.h>

#define TR_DESC1 TL("server", \
    "A multiplexing terminal emulator server with many features", "desc1")
#define TR_DESC2 TL("server", \
    "Establish a connection between terminal emulator servers", "desc2")
#define TR_DESC3 TL("server", \
    "Query and set terminal attributes", "desc3")
#define TR_PARSE1 TL("server", "%1 option requires an argument", "parse1")
#define TR_PARSE2 TL("server", "Warning: unrecognized option '%1'", "parse2")
#define TR_PARSE3 TL("server", "No command specified", "parse3")
#define TR_PARSE4 TL("server", "Invalid environment variable rule '%s'", "parse4")

ArgParser *g_args;

void
ArgParser::handleAbout()
{
    puts("Copyright © 2018 " ORG_NAME "\n"
         "This is free software, made available under the terms of the GNU GPL, version 2\n"
         "This software comes with ABSOLUTELY NO WARRANTY, either expressed or implied");
    exit(0);
}

void
ArgParser::handleServerHelp()
{
    printf(SERVER_NAME " [OPTIONS...]\n\n%s\n\n"
           "--nofork     Do not fork into a daemon process after startup\n"
           "--nolisten   Do not listen for client connections on a local socket\n"
           "--nostdin    Do not treat stdin as a client connection\n"
           "--client     Hand off stdin to an already running " SERVER_NAME " instance\n"
           "--standalone Do not listen on or connect to a local socket\n"
#if USE_SYSTEMD
           "--activated  Run as a socket activated service\n"
#endif
#if USE_LIBGIT2
           "--nogit      Disable git-specific file monitoring support\n"
#endif
           "--rundir DIR Use runtime path DIR\n",
           TR_DESC1);
    puts("--help       Show this help\n"
         "--man        Launch man page\n"
         "--version    Show version information\n"
         "--about      Show license information and disclaimer of warranty");
    exit(0);
}

void
ArgParser::handleConnectHelp()
{
    printf(CONNECT_NAME " [OPTIONS...] [--] COMMAND ARG...\n\n%s\n\n"
           "-p,--pty     Run COMMAND in a pseudoterminal\n"
           "-P,--nopty   Do not run COMMAND in a pseudoterminal (the default)\n"
           "-r,--raw     Use raw (binary) protocol encoding (the default)\n"
           "-R,--noraw   Use term (ascii) protocol encoding\n"
           "-k,--keepalive SEC  Set keepalive timeout in seconds\n"
           "-d,--dir DIR        Start COMMAND in directory DIR\n"
           "-0,--arg0 ARG       Use ARG as argv0 when running COMMAND\n"
           "-t,--rundir DIR     Use runtime path DIR\n"
           "-n,--noosc   Do not query the local terminal emulator\n",
           TR_DESC2);
    puts("--help       Show this help\n"
         "--man        Launch man page\n"
         "--version    Show version information\n"
         "--about      Show license information and disclaimer of warranty");
    exit(0);
}

void
ArgParser::handleQueryHelp()
{
    printf(QUERY_NAME " get VARNAME\n"
           QUERY_NAME " set VARNAME VALUE\n"
           QUERY_NAME " clear VARNAME\n\n%s\n\n",
           TR_DESC3);
    puts("--help       Show this help\n"
         "--man        Launch man page\n"
         "--version    Show version information\n"
         "--about      Show license information and disclaimer of warranty");
    exit(0);
}

void
ArgParser::checkServerState()
{
#if !USE_SYSTEMD
    if (m_activated)
        throw ErrnoException(ENOTSUP);
#endif

    if ((!m_stdinput && m_client) || (!m_stdinput && m_standalone) ||
        (m_client && m_standalone) || (m_client && m_activated) ||
        (m_standalone && m_activated) || (!m_listen && m_activated) ||
        (!m_listen && !m_stdinput))
    {
        throw ErrnoException(EINVAL);
    }
}

void
ArgParser::parseServerArg(char *const *argv, int &pos, int limit)
{
    const char *arg = argv[pos];
    std::string tmp;

    if (!strcmp(arg, "--nofork"))
        m_fork = false;
    else if (!strcmp(arg, "--nolisten"))
        m_listen = false;
    else if (!strcmp(arg, "--nostdin"))
        m_stdinput = false;

    else if (!strcmp(arg, "--client")) {
        m_listen = false;
        m_client = true;
    }
    else if (!strcmp(arg, "--standalone")) {
        m_listen = false;
        m_standalone = true;
    }
    else if (!strcmp(arg, "--activated")) {
        m_fork = false;
        m_stdinput = false;
        m_activated = true;
        m_fdpurge = false;
    }
    else if (!strcmp(arg, "-t") || !strcmp(arg, "--rundir")) {
        if (pos < limit - 1 && strncmp(argv[pos + 1], "--", 2))
            m_rundir = argv[++pos];
        else {
            this->arg(tmp, TR_PARSE1, "-t/--rundir");
            throw TsqException("%s", tmp.c_str());
        }
    }

    else if (!strcmp(arg, "--help"))
        handleServerHelp();
    else if (!strcmp(argv[1], "--man"))
        execlp("man", "man", SERVER_NAME, NULL), _exit(127);
    else if (!strcmp(arg, "--nofdpurge"))
        m_fdpurge = false;
    else if (!strcmp(arg, "--nogit"))
        m_git = false;

    else {
        this->arg(tmp, TR_PARSE2, arg);
        fprintf(stderr, "%s\n", tmp.c_str());
    }
}

void
ArgParser::checkConnectState()
{
    if (m_cmd.empty())
        throw TsqException("%s", TR_PARSE3);
    else
        m_cmd.pop_back(); // remove extra nul

    if (!m_env.empty())
        for (size_t pos1 = 0, pos2 = 0, n = m_env.size(); pos1 <= n; ++pos1)
        {
            if (m_env[pos1] != ',' && m_env[pos1] != '\0')
                continue;

            if (m_env[pos1] != '\0')
                m_env[pos1] = '\0';

            const char *str = m_env.data() + pos2;

            if ((*str != '-' && *str != '+') || (*str == '+' && !strchr(str, '='))) {
                std::string tmp;
                this->arg(tmp, TR_PARSE4, str);
                throw TsqException("%s", tmp.c_str());
            }

            pos2 = pos1 + 1;
        }
}

void
ArgParser::parseConnectArg(char *const *argv, int &pos, int limit)
{
    const char *arg = argv[pos];
    std::string tmp;
    int atend = (pos >= limit - 1) || !strncmp(argv[pos + 1], "--", 2);

    if (!strcmp(arg, "-d") || !strcmp(arg, "--dir")) {
        if (!atend)
            m_dir = argv[++pos];
        else {
            this->arg(tmp, TR_PARSE1, "-d/--dir");
            throw TsqException("%s", tmp.c_str());
        }
    }
    else if (!strcmp(arg, "-0") || !strcmp(arg, "--arg0")) {
        if (!atend)
            m_arg0 = argv[++pos];
        else {
            this->arg(tmp, TR_PARSE1, "-0/--arg0");
            throw TsqException("%s", tmp.c_str());
        }
    }
    else if (!strcmp(arg, "-t") || !strcmp(arg, "--rundir")) {
        if (!atend)
            m_rundir = argv[++pos];
        else {
            this->arg(tmp, TR_PARSE1, "-t/--rundir");
            throw TsqException("%s", tmp.c_str());
        }
    }
    else if (!strcmp(arg, "-k") || !strcmp(arg, "--keepalive")) {
        if (!atend)
            m_keepalive = atoi(argv[++pos]) * 1000;
        else {
            this->arg(tmp, TR_PARSE1, "-k/--keepalive");
            throw TsqException("%s", tmp.c_str());
        }
    }
    else if (!strcmp(arg, "-p") || !strcmp(arg, "--pty")) {
        m_pty = true;
    }
    else if (!strcmp(arg, "-P") || !strcmp(arg, "--nopty")) {
        m_pty = false;
    }
    else if (!strcmp(arg, "-r") || !strcmp(arg, "--raw")) {
        m_raw = true;
    }
    else if (!strcmp(arg, "-R") || !strcmp(arg, "--noraw")) {
        m_raw = false;
    }
    else if (!strcmp(arg, "-n") || !strcmp(arg, "--noosc")) {
        m_query = false;
    }

    else if (!strcmp(arg, "--help"))
        handleConnectHelp();
    else if (!strcmp(argv[1], "--man"))
        execlp("man", "man", CONNECT_NAME, NULL), _exit(127);

    else {
        // Treat everything further as command
        if (!strcmp(arg, "--")) {
            ++pos;
        }
        for (; pos < limit; ++pos) {
            m_cmd.append(argv[pos]);
            m_cmd.push_back('\0');
        }
    }
}

inline void
ArgParser::checkQueryState()
{
    if (m_env.empty())
        throw ErrnoException(EINVAL);
}

void
ArgParser::parseQueryArgs(char *const *argv, int limit)
{
    switch (limit) {
    case 4:
        if (!strcmp(argv[1], "set")) {
            m_cmd = argv[1];
            m_env = argv[2];
            m_arg0 = argv[3];
        }
        break;
    case 3:
        if (!strcmp(argv[1], "get") || !strcmp(argv[1], "clear")) {
            m_cmd = argv[1];
            m_env = argv[2];
        }
        break;
    case 2:
        if (!strcmp(argv[1], "--help"))
            handleQueryHelp();
        else if (!strcmp(argv[1], "--man"))
            execlp("man", "man", QUERY_NAME, NULL), _exit(127);
    }
}

void
ArgParser::parse(int argc, char **argv)
{
    // Load the default translator
    if (pthread_mutex_init(&m_lock, NULL) < 0)
        throw ErrnoException("pthread_mutex_init", errno);

    const char *lang = osGetLang();
    const Translator *translator = makeTranslator(lang);

    if (translator) {
        m_translators.emplace(lang, translator);
        m_translator = new Translator(*translator);
    } else {
        m_translator = new Translator();
    }

    m_translators.emplace(g_mtstr, m_translator);

    // Parse arguments
    m_progname = *argv;

    char *ptr = strstr(m_progname, ABBREV_NAME);
    if (!ptr)
        ptr = m_progname;

    if (argc > 1) {
        if (!strcmp(argv[1], "--about"))
            handleAbout();
        if (!strcmp(argv[1], "--version"))
            osPrintVersionAndExit(ptr);
    }

    m_isConnector = !strcmp(ptr, CONNECT_NAME);
    m_isQuery = !strcmp(ptr, QUERY_NAME);

    if (m_isConnector) {
        m_keepalive = KEEPALIVE_DEFAULT;

        for (int i = 1; i < argc; ++i)
            parseConnectArg(argv, i, argc);

        checkConnectState();
    }
    else if (m_isQuery) {
        parseQueryArgs(argv, argc);
        checkQueryState();
    }
    else {
        for (int i = 1; i < argc; ++i)
            parseServerArg(argv, i, argc);

        checkServerState();
    }
}

void
ArgParser::renameProcess(const char *name)
{
    char *ptr = strstr(m_progname, ABBREV_NAME);
    if (!ptr)
        ptr = m_progname;

    int i = 0, n = strlen(ptr);

    while (i < n && name[i]) {
        ptr[i] = name[i];
        ++i;
    }
    while (i < n) {
        ptr[i] = '\0';
        ++i;
    }
}

const char *
ArgParser::rundir() const
{
    return !m_rundir.empty() ? m_rundir.c_str() : SERVER_TMP_DIR;
}
