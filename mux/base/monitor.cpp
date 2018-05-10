// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "monitor.h"
#include "listener.h"
#include "zombies.h"
#include "os/attr.h"
#include "os/process.h"
#include "os/wait.h"
#include "os/signal.h"
#include "os/logging.h"
#include "config.h"

#include <cerrno>
#include <unistd.h>

TermMonitor *g_monitor;

TermMonitor::TermMonitor() :
    ThreadBase("monitor", ThreadBaseFd)
{
}

void
TermMonitor::addSpec(std::string &key, std::string &value)
{
    if (m_state)
        m_map.emplace(std::move(key), std::move(value));
    else if (!osRestrictedMonitorAttribute(key))
        g_listener->commandSetAttribute(key, value);
}

void
TermMonitor::removeSpec(std::string &key)
{
    if (m_state == 0 && !osRestrictedMonitorAttribute(key))
        g_listener->commandRemoveAttribute(key);
}

inline void
TermMonitor::startMonitor()
{
    try {
        m_fd = osForkMonitor(&m_pid);
    }
    catch (const std::exception &e) {
        LOGWRN("Monitor: failed to start process: %s\n", e.what());
        m_fd = -1;
        m_pid = 0;
        return;
    }

    LOGDBG("Monitor: started monitor process (pid %d)\n", m_pid);
    g_reaper->ignoreProcess(m_pid);
}

void
TermMonitor::stateChange()
{
    if (osAttributesAsync(m_map, &m_fd, &m_pid, &m_state)) {
        // Finished running scripts
        g_listener->commandSetAttributes(m_map);
        m_map.clear();
        startMonitor();
    }
    else if (m_pid) {
        LOGDBG("Monitor: started script process %d (pid %d)\n", m_state, m_pid);
        g_reaper->ignoreProcess(m_pid);
    }

    loadfd();
}

bool
TermMonitor::handleFd()
{
    char buf[ATTRIBUTE_MAX_LENGTH / 2];

    ssize_t rc = read(m_fd, buf, sizeof(buf));
    if (rc < 0) {
        if (errno == EINTR)
            return true;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;

        LOGWRN("Monitor: read error %d\n", errno);
        goto finish;
    }

    if (rc != 0)
        m_accum.append(buf, rc);
    else if (!m_accum.empty())
        m_accum.push_back('\n');
    else
        goto skip;

    while (1) {
        size_t idx = m_accum.find('\0');
        if (idx != std::string::npos)
            m_accum.erase(idx);

        idx = m_accum.find_first_of("\n\r", 0, 2);
        if (idx == std::string::npos)
            break;

        size_t edx = m_accum.find('=');
        if (edx != std::string::npos && edx && edx < idx) {
            std::string key(m_accum.substr(0, edx));
            std::string value(m_accum.substr(edx + 1, idx - edx - 1));
            addSpec(key, value);
        }
        else if (idx > 0) {
            std::string key(m_accum.substr(0, idx));
            removeSpec(key);
        }

        idx = m_accum.find_first_not_of("\n\r", idx, 2);
        if (idx == std::string::npos)
            m_accum.clear();
        else
            m_accum.erase(0, idx);
    }
skip:
    if (rc == 0) {
        goto finish;
    }
    if (m_accum.size() >= ATTRIBUTE_MAX_LENGTH) {
        LOGERR("Monitor: key-value size limit exceeded\n");
        m_accum.clear();
        goto finish;
    }
    return true;

finish:
    closefd();
    if (m_state) {
        LOGDBG("Monitor: script process %d (pid %d) finished\n", m_state, m_pid);
        stateChange();
    } else {
        LOGDBG("Monitor: monitor process (pid %d) finished\n", m_pid);
        m_pid = 0;
    }
    return true;
}

void
TermMonitor::handleRestart()
{
    if (m_pid) {
        g_reaper->abandonProcess(m_pid);
        osKillProcess(m_pid);
        closefd();
        LOGDBG("Monitor: sent SIGTERM to pid %d (reloading)\n", m_pid);
        LOGDBG("Monitor: abandoned pid %d (reloading)\n", m_pid);
    }

    m_map.clear();
    m_state = 0;
    stateChange();
}

void
TermMonitor::handleInput(std::string *data)
{
    if (m_state == 0) {
        data->push_back('\n');
        // Reliable delivery of monitor input is not guaranteed
        write(m_fd, data->data(), data->size());
    }
    delete data;
}

bool
TermMonitor::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case MonitorClose:
        return false;
    case MonitorRestart:
        handleRestart();
        break;
    case MonitorInput:
        handleInput((std::string *)item.value);
        break;
    default:
        break;
    }

    return true;
}

void
TermMonitor::threadMain()
{
    try {
        startMonitor();
        runDescriptorLoop();
    }
    catch (const std::exception &e) {
        LOGERR("Monitor: caught exception: %s\n", e.what());
    }

    closefd();

    if (m_pid) {
        osKillProcess(m_pid);
        LOGDBG("Monitor: sent SIGTERM to pid %d (server shutdown)\n", m_pid);
    }

    if (s_deathSignal)
        LOGDBG("Monitor: exiting on signal %d\n", s_deathSignal);
    else
        LOGDBG("Monitor: exiting\n");
}
