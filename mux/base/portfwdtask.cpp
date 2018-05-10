// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "portfwdtask.h"
#include "listener.h"
#include "os/logging.h"
#include "lib/wire.h"
#include "lib/protocol.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <cerrno>
#include <unistd.h>
#include <netdb.h>

#define HEADERSIZE 64

PortBase::PortBase(const char *name, Tsq::ProtocolUnmarshaler *unm, unsigned flags) :
    TaskBase(name, unm, flags|TaskBaseThrottlable|ThreadBaseMulti)
{
    m_chunkSize = unm->parseNumber();
    m_windowSize = unm->parseNumber();

    switch (unm->parseNumber()) {
    case Tsq::PortForwardTCP:
        m_type = Tsq::PortForwardTCP;
        m_address = unm->parseString();
        m_port = unm->parseString();
        break;
    case Tsq::PortForwardUNIX:
        m_type = Tsq::PortForwardUNIX;
        m_address = unm->parseString();
        break;
    default:
        break;
    }

    uint32_t command = htole32(TSQ_TASK_OUTPUT);

    m_buf = new char[m_chunkSize + HEADERSIZE + 4];
    memcpy(m_buf, &command, 4);
    memcpy(m_buf + 8, m_clientId.buf, 16);
    memcpy(m_buf + 24, g_listener->id().buf, 16);
    memcpy(m_buf + 40, m_taskId.buf, 16);
    m_ptr = m_buf + HEADERSIZE;
}

PortBase::~PortBase()
{
    delete [] m_buf;
}

void
PortBase::watchReads(bool enabled)
{
    for (unsigned i = 1; i < m_fds.size(); ++i)
        if (enabled)
            m_fds[i].events |= POLLIN;
        else
            m_fds[i].events &= ~POLLIN;
}

void
PortBase::pushStart(portfwd_t id, const char *host, const char *serv)
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addBytes(m_buf + 8, 48);
    m.addNumberPair(Tsq::TaskStarting, id);
    m.addStringPair(host, serv);

    if (!throttledOutput(m.result())) {
        LOGDBG("PortFwd %p: throttled (local)\n", this);
        m_throttled = true;
        watchReads(false);
    }
}

void
PortBase::pushBytes(portfwd_t id, size_t len)
{
    uint32_t num = htole32(HEADERSIZE - 8 + len);
    memcpy(m_buf + 4, &num, 4);
    num = htole32(Tsq::TaskRunning);
    memcpy(m_buf + 56, &num, 4);
    num = htole32(id);
    memcpy(m_buf + 60, &num, 4);

    std::string buf(m_buf, HEADERSIZE + len);
    m_sent += len;

    if (!throttledOutput(buf)) {
        LOGDBG("PortFwd %p: throttled (local)\n", this);
        m_throttled = true;
        watchReads(false);
    }
}

void
PortBase::pushAck()
{
    uint32_t num = htole32(HEADERSIZE - 4);
    memcpy(m_buf + 4, &num, 4);
    num = htole32(Tsq::TaskAcking);
    memcpy(m_buf + 56, &num, 4);
    uint64_t num2 = htole64(m_received);
    memcpy(m_buf + 60, &num2, 8);

    std::string buf(m_buf, HEADERSIZE + 4);

    if (!throttledOutput(buf)) {
        LOGDBG("PortFwd %p: throttled (local)\n", this);
        m_throttled = true;
        watchReads(false);
    }
}

void
PortBase::closefd(portfwd_t id)
{
    auto k = m_idmap.find(id);

    for (unsigned i = 1; i < m_fds.size(); ++i)
        if (m_fds[i].fd == k->second->fd) {
            m_fds.erase(m_fds.begin() + i);
            break;
        }

    m_fdmap.erase(k->second->fd);
    close(k->second->fd);
    freeaddrinfo(k->second->a);

    // Clean up data strings
    {
        Lock lock(this);
        while (!k->second->outdata.empty()) {
            std::string *data = k->second->outdata.front();
            k->second->outdata.pop();
            m_incomingData.erase(data);
            delete data;
        }
    }

    delete k->second;
    m_idmap.erase(k);
}

inline void
PortBase::watchWrites(int fd, bool enabled)
{
    for (unsigned i = 1; i < m_fds.size(); ++i)
        if (m_fds[i].fd == fd) {
            if (enabled)
                m_fds[i].events |= POLLOUT;
            else
                m_fds[i].events &= ~POLLOUT;

            break;
        }
}

bool
PortBase::handleBytes(portfwd_t id, std::string *data)
{
    auto i = m_idmap.find(id);
    if (i == m_idmap.end()) {
        LOGDBG("PortFwd %p: Ignoring invalid id %u\n", this, id);
        return true;
    }

    auto *cstate = i->second;
    size_t len = data->size() - 8;
    ssize_t rc;

    if (!cstate->outdata.empty() || cstate->special || len == 0)
        goto push;

    // Try a quick write
    rc = write(cstate->fd, data->data() + 8, len);
    if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            goto push;

        // Close this connection
        LOGDBG("PortFwd %p: %u: write: %m\n", this, id);
        closefd(id);
        pushBytes(id, 0);
        return false;
    }

    m_received += rc;
    if (m_chunks < m_received / m_chunkSize) {
        m_chunks = m_received / m_chunkSize;
        pushAck();
    }

    if (rc == len)
        return true;

    data->erase(8, rc);
push:
    cstate->outdata.push(data);
    watchWrites(cstate->fd, true);
    return false;
}

void
PortBase::handleData(std::string *data)
{
    Tsq::ProtocolUnmarshaler unm(data->data(), data->size());

    switch (unm.parseNumber()) {
    case Tsq::TaskRunning:
        if (!handleBytes(unm.parseNumber(), data)) {
            // Keep data in incoming pool until written
            // Or, the connection was closed
            return;
        }
        break;
    case Tsq::TaskAcking:
        m_acked = unm.parseNumber64();

        if (!m_running) {
            m_running = true;
            watchReads(!m_throttled);
        }
        break;
    case Tsq::TaskStarting:
        handleStart(unm.parseNumber());
        break;
    }
    {
        Lock lock(this);
        m_incomingData.erase(data);
    }
    delete data;
}

void
PortBase::writefd(pollfd &pfd, PortFwdState *cstate)
{
    if (cstate->outdata.empty()) {
        pfd.events &= ~POLLOUT;
        return;
    }

    std::string *data = cstate->outdata.front();
    size_t len = data->size() - 8;
    portfwd_t id = cstate->id;
    ssize_t rc;

    if (len == 0) {
        // Writing finished
        LOGDBG("PortFwd %p: %u: remote eof\n", this, id);
        closefd(id);
        return;
    }
    else if ((rc = write(pfd.fd, data->data() + 8, len)) == len) {
        {
            Lock lock(this);
            m_incomingData.erase(data);
        }
        cstate->outdata.pop();
        delete data;
    }
    else if (rc < 0) {
        if (errno != EAGAIN && errno != EINTR) {
            // Close this connection
            LOGDBG("PortFwd %p: %u: write: %m\n", this, id);
            closefd(id);
            pushBytes(id, 0);
        }
        return;
    }
    else {
        data->erase(8, rc);
    }

    m_received += rc;
    if (m_chunks < m_received / m_chunkSize) {
        m_chunks = m_received / m_chunkSize;
        pushAck();
    }
}

void
PortBase::readfd(pollfd &pfd, PortFwdState *cstate)
{
    portfwd_t id = cstate->id;
    ssize_t rc = read(pfd.fd, m_ptr, m_chunkSize);

    if (rc > 0) {
        // LOGDBG("PortFwd %p: %u: read %zu bytes\n", this, id, rc);
    } else {
        if (rc == 0) {
            LOGDBG("PortFwd %p: %u: local eof\n", this, id);
        } else if (errno == EAGAIN || errno == EINTR) {
            return;
        } else {
            LOGDBG("PortFwd %p: %u: read: %m\n", this, id);
            rc = 0;
        }
        // Close this connection
        closefd(id);
    }

    pushBytes(id, rc);
}

bool
PortBase::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case TaskClose:
        LOGDBG("PortFwd %p: canceled (code %td)\n", this, item.value);
        return false;
    case TaskInput:
        handleData((std::string *)item.value);
        break;
    case TaskPause:
        m_throttled = true;
        watchReads(false);
        LOGDBG("PortFwd %p: throttled (remote)\n", this);
        break;
    case TaskResume:
        m_throttled = false;
        pushAck();
        watchReads(m_running);
        LOGDBG("PortFwd %p: resumed\n", this);
        break;
    default:
        break;
    }

    return true;
}

void
PortBase::handleStart(portfwd_t)
{}
