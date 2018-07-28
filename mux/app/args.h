// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/attributemap.h"

#include <pthread.h>

class Translator;
#define TL(cat, str, key) m_translator->get(key, str)

class ArgParser
{
private:
    bool m_isConnector;
    bool m_isQuery;

    // Server
    bool m_fork = true;
    bool m_listen = true;
    bool m_stdinput = true;
    bool m_client = false;
    bool m_standalone = false;
    bool m_activated = false;
    bool m_fdpurge = true;
    bool m_git = true;

    // Connector
    bool m_pty = false;
    bool m_raw = true;
    bool m_query = true;
    unsigned m_keepalive;
    std::string m_cmd;
    std::string m_dir;
    std::string m_env;
    std::string m_arg0;

    // Global
    std::string m_rundir;

    void handleServerHelp() __attribute__((noreturn));
    void handleConnectHelp() __attribute__((noreturn));
    void handleQueryHelp() __attribute__((noreturn));
    void handleAbout() __attribute__((noreturn));

    void parseServerArg(char *const *argv, int &pos, int limit);
    void checkServerState();
    void parseConnectArg(char *const *argv, int &pos, int limit);
    void checkConnectState();
    void parseQueryArgs(char *const *argv, int limit);
    void checkQueryState();

    char *m_progname;

public:
    ~ArgParser();
    void parse(int argc, char **argv);

    inline bool isConnector() const { return m_isConnector; }
    inline bool isQuery() const { return m_isQuery; }

    // Server
    inline bool fork() const { return m_fork; }
    inline bool listen() const { return m_listen; }
    inline bool stdinput() const { return m_stdinput; }
    inline bool client() const { return m_client; }
    inline bool standalone() const { return m_standalone; }
    inline bool activated() const { return m_activated; }
    inline bool fdpurge() const { return m_fdpurge; }
    inline bool git() const { return m_git; }

    // Connector
    inline bool pty() const { return m_pty; }
    inline bool raw() const { return m_raw; }
    inline bool query() const { return m_query; }
    inline unsigned keepalive() const { return m_keepalive; }
    inline const std::string& cmd() const { return m_cmd; }
    inline const std::string& dir() const { return m_dir; }
    inline const std::string& env() const { return m_env; }
    inline const std::string& arg0() const { return m_arg0; }

    // Global
    const char* rundir() const;

    void renameProcess(const char *name);

private:
    // Translations
    pthread_mutex_t m_lock;
    std::unordered_map<std::string,const Translator*> m_translators;
    const Translator *m_translator = nullptr;

    Translator* makeTranslator(std::string lang);

    // RAII locking interface
    class Lock {
        ArgParser *m_t;
    public:
        Lock(ArgParser *t);
        ~Lock();
    };

public:
    inline void disableGit() { m_git = false; }

    void loadTranslator();
    const Translator* getTranslator(const std::string &lang);
    inline const Translator* defaultTranslator() const { return m_translator; }

    static void arg(std::string &str, const char *fmt, int arg1, int arg2 = 0);
    static void arg(std::string &str, const char *fmt, const char *arg);

    int printExitStatus(int code);
    int printConnectError(int errtype, int errnum, int errsave);
};

class Translator final: public StringMap
{
public:
    std::string path;
    const char* get(const std::string &key, const char *defval) const;
};

extern ArgParser *g_args;
