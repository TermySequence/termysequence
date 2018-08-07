// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/exception.h"
#include "app/logging.h"
#include "portouttask.h"
#include "conn.h"
#include "listener.h"
#include "os/conn.h"
#include "os/user.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/sequences.h"

#include <QSocketNotifier>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#define BUFSIZE (16 * TERM_PAYLOADSIZE)
#define HEADERSIZE 64
#define PAYLOADSIZE (BUFSIZE - HEADERSIZE)
#define WINDOWSIZE 8

PortFwdTask::PortFwdTask(ServerInstance *server, const PortFwdRule &config) :
    TermTask(server, true),
    m_config(config)
{
    uint32_t command = htole32(TSQ_TASK_INPUT);

    m_buf = new char[BUFSIZE];
    memcpy(m_buf, &command, 4);
    memcpy(m_buf + 8, server->id().buf, 16);
    memcpy(m_buf + 24, g_listener->id().buf, 16);
    memcpy(m_buf + 40, m_taskId.buf, 16);
    m_ptr = m_buf + HEADERSIZE;
}

void
PortFwdTask::closefd(portfwd_t id)
{
    auto k = m_idmap.find(id);

    m_fdmap.erase(k->second->fd);
    delete k->second->reader;
    delete k->second->writer;
    freeaddrinfo(k->second->a);
    close(k->second->fd);
    delete k->second;
    m_idmap.erase(k);
    emit connectionRemoved(id);
}

void
PortFwdTask::closefds()
{
    for (auto &&i: m_fdmap) {
        delete i.second->reader;
        delete i.second->writer;
        freeaddrinfo(i.second->a);
        close(i.second->fd);
        delete i.second;
    }

    m_fdmap.clear();
    m_idmap.clear();
}

PortFwdTask::~PortFwdTask()
{
    closefds();
    delete [] m_buf;
}

void
PortFwdTask::pushStart(portfwd_t id)
{
    uint32_t num = htole32(HEADERSIZE - 8);
    memcpy(m_buf + 4, &num, 4);
    num = htole32(Tsq::TaskStarting);
    memcpy(m_buf + 56, &num, 4);
    num = htole32(id);
    memcpy(m_buf + 60, &num, 4);
    m_server->conn()->push(m_buf, HEADERSIZE);
}

void
PortFwdTask::pushBytes(portfwd_t id, size_t len)
{
    uint32_t num = htole32(HEADERSIZE - 8 + len);
    memcpy(m_buf + 4, &num, 4);
    num = htole32(Tsq::TaskRunning);
    memcpy(m_buf + 56, &num, 4);
    num = htole32(id);
    memcpy(m_buf + 60, &num, 4);
    m_server->conn()->push(m_buf, HEADERSIZE + len);

    m_sent += len;
    queueTaskChange();
}

void
PortFwdTask::pushAck()
{
    uint32_t num = htole32(HEADERSIZE - 4);
    memcpy(m_buf + 4, &num, 4);
    num = htole32(Tsq::TaskAcking);
    memcpy(m_buf + 56, &num, 4);
    uint64_t num2 = htole64(m_received);
    memcpy(m_buf + 60, &num2, 8);
    m_server->conn()->push(m_buf, HEADERSIZE + 4);
}

void
PortFwdTask::start(TermManager *manager, uint32_t command)
{
    if (TermTask::doStart(manager)) {
        // Write task start
        Tsq::ProtocolMarshaler m(command);
        m.addBytes(m_buf + 8, 48);
        m.addNumberPair(PAYLOADSIZE, WINDOWSIZE);
        m.addNumber(m_config.rtype);
        m.addString(m_config.raddr);
        if (m_config.rtype == Tsq::PortForwardTCP)
            m.addBytes(m_config.rport);

        m_server->conn()->push(m.resultPtr(), m.length());
    } else {
        closefds();
    }
}

inline void
PortFwdTask::watchReads(bool enabled)
{
    for (const auto &i: m_fdmap)
        i.second->reader->setEnabled(enabled);
}

void
PortFwdTask::handleRead(int fd)
{
    if (m_sent - m_acked >= WINDOWSIZE * PAYLOADSIZE) {
        // Stop and wait for ack message
        m_running = false;
        watchReads(false);
        return;
    }

    portfwd_t id = m_fdmap[fd]->id;
    ssize_t rc = read(fd, m_ptr, PAYLOADSIZE);

    if (rc > 0) {
        // logDebug(lcCommand, "PortFwd %p: %u: read %zu bytes", this, id, rc);
    } else {
        if (rc == 0) {
            logDebug(lcCommand, "PortFwd %p: %u: local eof", this, id);
        } else if (errno == EAGAIN || errno == EINTR) {
            return;
        } else {
            logDebug(lcCommand, "PortFwd %p: %u: read: %s", this, id, strerror(errno));
            rc = 0;
        }
        // Close this connection
        closefd(id);
    }

    pushBytes(id, rc);
}

void
PortFwdTask::handleWrite(int fd)
{
    auto *cstate = m_fdmap[fd];
    if (cstate->outdata.empty()) {
        cstate->writer->setEnabled(false);
        return;
    }

    QByteArray &data = cstate->outdata.front();
    size_t len = data.size();
    portfwd_t id = cstate->id;
    ssize_t rc;

    if (len == 0) {
        // Writing finished
        logDebug(lcCommand, "PortFwd %p: %u: remote eof", this, id);
        closefd(id);
        return;
    }
    else if ((rc = write(fd, data.data(), len)) == len) {
        cstate->outdata.pop();
    }
    else if (rc < 0) {
        if (errno != EAGAIN && errno != EINTR) {
            // Close this connection
            logDebug(lcCommand, "PortFwd %p: %u: write: %s", this, id, strerror(errno));
            closefd(id);
            pushBytes(id, 0);
        }
        return;
    }
    else {
        data.remove(0, rc);
    }

    m_received += rc;
    if (m_chunks < m_received / PAYLOADSIZE) {
        m_chunks = m_received / PAYLOADSIZE;
        pushAck();
    }
    queueTaskChange();
}

void
PortFwdTask::handleAccept(int fd)
{
    if (m_sent - m_acked >= WINDOWSIZE * PAYLOADSIZE) {
        // Stop and wait for ack message
        m_running = false;
        watchReads(false);
        return;
    }

    int cfd;
    char buf[512];
    struct sockaddr *ptr = (struct sockaddr *)buf;
    socklen_t len = sizeof(buf);

    try {
        cfd = osAccept(fd, ptr, &len);
    }
    catch (const std::exception &) {
        logDebug(lcCommand, "PortFwd %p: accept error", this);
        // Close this listener
        auto *lstate = m_fdmap[fd];
        delete lstate->reader;
        delete lstate;
        m_fdmap.erase(fd);
        close(fd);
        return;
    }

    if (cfd < 0)
        return;
    ++m_nextId;
    m_nextId += (m_nextId == INVALID_PORTFWD);

    logDebug(lcCommand, "PortFwd %p: %u: accepted", this, m_nextId);
    PortFwdState *cstate = new PortFwdState{ m_nextId, cfd };
    cstate->reader = new QSocketNotifier(cfd, QSocketNotifier::Read);
    connect(cstate->reader, &QSocketNotifier::activated, this, &PortFwdTask::handleRead);
    cstate->writer = new QSocketNotifier(cfd, QSocketNotifier::Write);
    cstate->writer->setEnabled(false);
    connect(cstate->writer, &QSocketNotifier::activated, this, &PortFwdTask::handleWrite);

    m_fdmap[cfd] = cstate;
    m_idmap[m_nextId] = cstate;

    if (m_config.ltype == Tsq::PortForwardTCP) {
        char host[64], serv[32];
        if (getnameinfo(ptr, len, host, 64, serv, 32, NI_NUMERICHOST|NI_NUMERICSERV))
            host[0] = serv[0] = '\0';
        cstate->caddr = host;
        cstate->cport = serv;
    }
    else {
        int uid, pid;
        if (osLocalCreds(cfd, uid, pid)) {
            cstate->cport = osUserName(uid);
            cstate->cport.push_back(':');
            cstate->cport.append(std::to_string(pid));
        }
    }
    pushStart(m_nextId);
    emit connectionAdded(m_nextId);
}

void
PortFwdTask::handleBytes(portfwd_t id, const char *buf, size_t len)
{
    auto i = m_idmap.find(id);
    if (i == m_idmap.end()) {
        logDebug(lcCommand, "PortFwd %p: ignoring invalid id %u", this, id);
        return;
    }

    auto *cstate = i->second;
    ssize_t rc;

    if (!cstate->outdata.empty() || cstate->special || len == 0)
        goto push;

    // Try a quick write
    rc = write(cstate->fd, buf, len);
    if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            goto push;

        // Close this connection
        closefd(id);
        pushBytes(id, 0);
        logDebug(lcCommand, "PortFwd %p: %u: write error", this, id);
        return;
    }

    m_received += rc;
    if (m_chunks < m_received / PAYLOADSIZE) {
        m_chunks = m_received / PAYLOADSIZE;
        pushAck();
    }
    queueTaskChange();

    if (rc == len)
        return;

    buf += rc;
    len -= rc;
push:
    cstate->outdata.emplace(buf, len);
    cstate->writer->setEnabled(true);
}

void
PortFwdTask::handleStart(Tsq::ProtocolUnmarshaler *)
{
}

void
PortFwdTask::handleOutput(Tsq::ProtocolUnmarshaler *unm)
{
    if (finished())
        return;

    int status = unm->parseNumber();
    portfwd_t id;

    switch (status) {
    case Tsq::TaskAcking:
        m_acked = unm->parseNumber64();
        m_running = true;
        watchReads(!m_throttled);
        break;
    case Tsq::TaskRunning:
        id = unm->parseNumber();
        handleBytes(id, unm->remainingBytes(), unm->remainingLength());
        break;
    case Tsq::TaskStarting:
        handleStart(unm);
        break;
    case Tsq::TaskError:
        closefds();
        unm->parseNumber();
        fail(QString::fromStdString(unm->parseString()));
        break;
    default:
        break;
    }
}

void
PortFwdTask::cancel()
{
    closefds();

    // Write task cancel
    if (!finished()) {
        Tsq::ProtocolMarshaler m(TSQ_CANCEL_TASK);
        m.addBytes(m_buf + 8, 48);
        m_server->conn()->push(m.resultPtr(), m.length());
    }

    TermTask::cancel();
}

void
PortFwdTask::handleDisconnect()
{
    closefds();
}

void
PortFwdTask::handleThrottle(bool throttled)
{
    m_throttled = throttled;
    watchReads(!throttled && m_running);
}

bool
PortFwdTask::clonable() const
{
    return finished() && m_target;
}

PortFwdTask::ConnInfo
PortFwdTask::connectionInfo(portfwd_t id) const
{
    ConnInfo result;
    auto k = m_idmap.find(id);
    result.caddr = QString::fromStdString(k->second->caddr);
    result.cport = QString::fromStdString(k->second->cport);
    result.id = id;
    return result;
}

QVector<PortFwdTask::ConnInfo>
PortFwdTask::connectionsInfo() const
{
    QVector<ConnInfo> result;
    for (const auto &i: m_idmap) {
        ConnInfo item;
        item.caddr = QString::fromStdString(i.second->caddr);
        item.cport = QString::fromStdString(i.second->cport);
        item.id = i.first;
        result.push_back(item);
    }
    return result;
}

void
PortFwdTask::killConnection(portfwd_t id)
{
    auto k = m_idmap.find(id);
    if (k != m_idmap.end()) {
        // Close this connection
        closefd(id);
        pushBytes(id, 0);
        logDebug(lcCommand, "PortFwd %p: %u: killed", this, id);
    }
}
