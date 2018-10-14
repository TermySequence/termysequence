// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "attr.h"
#include "dir.h"
#include "fd.h"
#include "process.h"
#include "time.h"
#include "lib/attr.h"
#include "lib/attrstr.h"
#include "config.h"
#include "gitinfo.h"

#include <cstdio>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include <set>
using namespace std::string_literals;

#define FALLBACK_USER "unknown"
#define FALLBACK_HOST "unknown"
#define FALLBACK_NAME "unknown"
#define FALLBACK_ICON "computer"
#define FALLBACK_HOME "/"

static const std::string s_restrictedMonitorAttributes[] = {
    TSQ_ATTR_ID,
    TSQ_ATTR_MACHINE_ID,
    TSQ_ATTR_STARTED,
    TSQ_ATTR_UID,
    TSQ_ATTR_GID,
    TSQ_ATTR_USER,
    TSQ_ATTR_GITDESC,
};

static const std::set<std::string> s_restrictedServerAttributes = {
    TSQ_ATTR_ID,
    TSQ_ATTR_HOST,
    TSQ_ATTR_ICON,
    TSQ_ATTR_MACHINE_ID,
    TSQ_ATTR_NAME,
    TSQ_ATTR_STARTED,
    TSQ_ATTR_PID,
    TSQ_ATTR_UID,
    TSQ_ATTR_GID,
    TSQ_ATTR_USER,
    TSQ_ATTR_USERFULL,
    TSQ_ATTR_HOME,
    TSQ_ATTR_GITDESC,
};

static const std::set<std::string> s_restrictedTermAttributes = {
    TSQ_ATTR_ID,
    TSQ_ATTR_PEER,
    TSQ_ATTR_COMMAND,
    TSQ_ATTR_STARTED,
    TSQ_ATTR_EMULATOR,
    TSQ_ATTR_ENCODING,
    TSQ_ATTR_CURSOR,
    TSQ_ATTR_SENDER_PREFIX,
    TSQ_ATTR_OWNER_PREFIX,
    TSQ_ATTR_PROC_PREFIX,
    TSQ_ATTR_SERVER_PREFIX,
    TSQ_ATTR_CLIPBOARD_PREFIX,
    TSQ_ATTR_ENV_PREFIX,
};

static const std::set<std::string> s_ownerTermAttributes = {
    TSQ_ATTR_PROFILE,
    TSQ_ATTR_PROFILE_PREFIX,
    TSQ_ATTR_PREF_PREFIX,
    TSQ_ATTR_SESSION_PREFIX,
};

bool
osRestrictedMonitorAttribute(const std::string &key)
{
    for (int i = 0; i < sizeof(s_restrictedMonitorAttributes)/sizeof(std::string); ++i)
        if (key == s_restrictedMonitorAttributes[i])
            return true;

    return false;
}

bool
osRestrictedServerAttribute(const std::string &key)
{
    size_t idx = key.find('.');

    if (idx != std::string::npos) {
        std::string tmp(key, 0, idx + 1);
        return s_restrictedServerAttributes.count(tmp);
    }

    return s_restrictedServerAttributes.count(key);
}

bool
osRestrictedTermAttribute(const std::string &key, bool isOwner)
{
    size_t idx = key.find('.');

    if (idx != std::string::npos) {
        std::string tmp(key, 0, idx + 1);
        return s_restrictedTermAttributes.count(tmp) ||
            (!isOwner && s_ownerTermAttributes.count(tmp));
    }

    return s_restrictedTermAttributes.count(key) ||
        (!isOwner && s_ownerTermAttributes.count(key));
}

static void
timedReadMulti(int fd, StringMap &map, bool nullsep = false)
{
    // Read until EOF, buffer size exceeded, or time limit exceeded
    int64_t timelimit = osMonotime() + ATTRIBUTE_SCRIPT_TIMEOUT;
    pollfd pfd = { .fd = fd, .events = POLLIN };
    char buf[ATTRIBUTE_MAX_LENGTH / 2];
    const char *seps = "\n\r" + (nullsep ? 2 : 0);
    size_t nseps = nullsep ? 1 : 2;
    std::string accum;
    ssize_t rc;

    do {
        int64_t timeout = timelimit - osMonotime();
        if (timeout <= 0)
            break;

        rc = poll(&pfd, 1, timeout);
        if (rc == 0 || (rc < 0 && errno != EAGAIN))
            break;

        rc = read(fd, buf, sizeof(buf));
        if (rc < 0) {
            if (errno == EAGAIN)
                continue;
            else
                break;
        }

        if (rc != 0)
            accum.append(buf, rc);
        else if (!accum.empty())
            accum.push_back(*seps);
        else
            break;

        while (1) {
            size_t idx;
            if (!nullsep && (idx = accum.find('\0')) != std::string::npos)
                accum.erase(idx);

            idx = accum.find_first_of(seps, 0, nseps);
            if (idx == std::string::npos)
                break;

            size_t edx = accum.find('=');
            if (edx != std::string::npos && edx && edx < idx)
                map[accum.substr(0, edx)] = accum.substr(edx + 1, idx - edx - 1);

            idx = accum.find_first_not_of(seps, idx, nseps);
            if (idx == std::string::npos)
                accum.clear();
            else
                accum.erase(0, idx);
        }
    }
    while (rc != 0 && accum.size() < sizeof(buf));
}

static bool
timedRead(int fd, std::string &result)
{
    // Read until EOF, buffer size exceeded, or time limit exceeded
    int64_t timelimit = osMonotime() + ATTRIBUTE_SCRIPT_TIMEOUT;
    pollfd pfd = { .fd = fd, .events = POLLIN };
    char buf[128], *ptr;
    ssize_t rem = sizeof(buf);
    ptr = buf;

    while (rem > 0) {
        int64_t timeout = timelimit - osMonotime();
        if (timeout <= 0)
            break;

        ssize_t rc = poll(&pfd, 1, timeout);
        if (rc == 0 || (rc < 0 && errno != EAGAIN))
            break;

        rc = read(fd, ptr, rem);
        if (rc < 0) {
            if (errno == EAGAIN)
                continue;
            else
                break;
        }
        if (rc == 0) {
            size_t i;
            for (i = 0; i < sizeof(buf) - rem; ++i)
                if (buf[i] == '\n' || buf[i] == '\r')
                    break;

            result.assign(buf, i);
            return !result.empty();
        }

        ptr += rc;
        rem -= rc;
    }

    return false;
}

bool
osLoadFile(const char *path, StringMap &map, bool nullsep)
{
    int fd;

    try {
        fd = osOpenFile(path, NULL, NULL);
    } catch (const std::exception &e) {
        return false;
    }

    timedReadMulti(fd, map, nullsep);
    close(fd);
    return true;
}

static bool
loadScript(const std::string &path, std::string &result, std::vector<int> &pids)
{
    struct stat info;
    const char *pathc = path.c_str();
    int fd, pid;

    if (stat(pathc, &info) < 0)
        return false;
    if (!S_ISREG(info.st_mode) || !(info.st_mode & S_IXUSR))
        return false;

    try {
        fd = osForkAttributes(pathc, &pid);
    } catch (const std::exception &e) {
        return false;
    }

    pids.push_back(pid);
    bool rc = timedRead(fd, result);
    close(fd);
    return rc;
}

static int
loadScriptMultiFd(const std::string &path, std::vector<int> &pids)
{
    struct stat info;
    const char *pathc = path.c_str();
    int fd, pid;

    if (stat(pathc, &info) < 0)
        return -1;
    if (!S_ISREG(info.st_mode) || !(info.st_mode & S_IXUSR))
        return -1;

    try {
        fd = osForkAttributes(pathc, &pid);
    } catch (const std::exception &e) {
        return -1;
    }

    pids.push_back(pid);
    return fd;
}

static void
loadScriptMulti(const std::string &path, StringMap &map, std::vector<int> &pids)
{
    int fd = loadScriptMultiFd(path, pids);
    if (fd != -1) {
        timedReadMulti(fd, map);
        close(fd);
    }
}

static int
loadMonitorMultiFd(std::vector<int> &pids)
{
    int fd, pid;

    try {
        fd = osForkAttributes(MONITOR_NAME " --initial", &pid);
    } catch (const std::exception &e) {
        return -1;
    }

    pids.push_back(pid);
    return fd;
}

static void
loadMonitorMulti(StringMap &map, std::vector<int> &pids)
{
    int fd = loadMonitorMultiFd(pids);
    if (fd != -1) {
        timedReadMulti(fd, map);
        close(fd);
    }
}

static void
fallbackUser(StringMap &map)
{
    uid_t uid = getuid();
    const struct passwd *ent;

    while ((ent = getpwent()) != NULL) {
        if (uid == ent->pw_uid) {
            const char *ptr = ent->pw_gecos;
            const char *rs = strchr(ptr, ',');
            size_t count = rs ? (rs - ptr) : strlen(ptr);

            map.emplace(Tsq::attr_USER, ent->pw_name);
            map.emplace(Tsq::attr_USERFULL, std::string(ptr, count));
            break;
        }
    }
    endpwent();

    const char *userc = getenv("USER");
    map.emplace(Tsq::attr_USER, userc ? userc : FALLBACK_USER);
}

void
osIdentity(Tsq::Uuid &result, std::vector<int> &pids)
{
    std::string path, str;

    if (osConfigPath(SERVER_NAME, "id-script", path) && loadScript(path, str, pids) &&
        result.parse(str.c_str()))
        return;
    path = CONFDIR "/" SERVER_NAME "/id-script";
    if (loadScript(path, str, pids) && result.parse(str.c_str()))
        return;
    path = PREFIX "/lib/" SERVER_NAME "/id-script";
    if (loadScript(path, str, pids) && result.parse(str.c_str()))
        return;

    struct stat info;
    FILE *file;
    char buf[128], *ptr = NULL;

    if (!(file = fopen("/etc/machine-id", "r")))
        goto fallback;
    if (fstat(fileno(file), &info) == 0 && S_ISREG(info.st_mode))
        ptr = fgets(buf, sizeof(buf), file);

    fclose(file);
    if (!ptr || !*ptr)
        goto fallback;

    ptr = buf + strlen(buf) - 1;
    while (ptr != buf && (*ptr == '\n' || *ptr == '\r'))
        *ptr-- = '\0';

    if (!result.parse(buf))
    fallback:
        osFallbackIdentity(result);
}

static void
osFallbackAttributes(StringMap &map, bool isServer)
{
    /*
     * Remove attributes that scripts aren't allowed to set
     */
    for (int i = 0; i < sizeof(s_restrictedMonitorAttributes)/sizeof(std::string); ++i)
        map.erase(s_restrictedMonitorAttributes[i]);

    /*
     * Server-only defaults
     */
    if (isServer) {
        const char *homec = getenv("HOME");
        map.emplace(Tsq::attr_HOME, homec ? homec : FALLBACK_HOME);
        map.emplace(Tsq::attr_GITDESC, GITDESC);
        map.emplace(Tsq::attr_ICON, FALLBACK_ICON);
        map.emplace(Tsq::attr_UID, std::to_string(getuid()));
        map.emplace(Tsq::attr_GID, std::to_string(getgid()));
        map.emplace(Tsq::attr_NAME, FALLBACK_NAME);
    }

    /*
     * Defaults
     */
    if (!map.count(Tsq::attr_USER) || !map.count(Tsq::attr_USERFULL)) {
        fallbackUser(map);
    }
    if (!map.count(Tsq::attr_HOST)) {
        char buf[256];
        buf[sizeof(buf) - 1] = '\0';
        map.emplace(Tsq::attr_HOST, gethostname(buf, sizeof(buf) - 1) == 0 ?
                    buf : FALLBACK_HOST);
    }
}

void
osAttributes(StringMap &map, std::vector<int> &pids, bool isServer)
{
    std::string path;
    const char *appname = isServer ? SERVER_NAME : APP_NAME;

    /*
     * Stock monitor initial attributes
     */
    if (isServer)
        loadMonitorMulti(map, pids);

    /*
     * Optional attribute scripts
     */
    path = PREFIX "/lib/";
    path += appname;
    path += "/attr-script";
    loadScriptMulti(path, map, pids);

    path = CONFDIR "/";
    path += appname;
    path += "/attr-script";
    loadScriptMulti(path, map, pids);

    if (osConfigPath(appname, "attr-script", path))
        loadScriptMulti(path, map, pids);

    osFallbackAttributes(map, isServer);
}

extern bool
osAttributesAsync(StringMap &map, int *fdret, int *pidret, int *state)
{
    std::string path;
    std::vector<int> pids(1, 0);
    int fd = -1;
    bool rc = false;

    switch (*state) {
    case 0:
        /*
         * Stock monitor initial attributes
         */
        *state = 1;
        fd = loadMonitorMultiFd(pids);
        if (fd != -1)
            break;
        // fallthru
    case 1:
        /*
         * Optional attribute scripts
         */
        *state = 2;
        path = PREFIX "/lib/" SERVER_NAME "/attr-script";
        fd = loadScriptMultiFd(path, pids);
        if (fd != -1)
            break;
        // fallthru
    case 2:
        *state = 3;
        path = CONFDIR "/" SERVER_NAME "/attr-script";
        fd = loadScriptMultiFd(path, pids);
        if (fd != -1)
            break;
        // fallthru
    case 3:
        *state = 4;
        if (osConfigPath(SERVER_NAME, "attr-script", path)) {
            fd = loadScriptMultiFd(path, pids);
            if (fd != -1)
                break;
        }
        // fallthru
    case 4:
        *state = 0;
        osFallbackAttributes(map, true);
        rc = true;
        break;
    }

    *fdret = fd;
    *pidret = pids.back();
    return rc;
}

void
osPrintVersionAndExit(const char *name)
{
    printf("%s " PROJECT_VERSION " (git: " GITDESC ")\n", name);
    exit(0);
}
