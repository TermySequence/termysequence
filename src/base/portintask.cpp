// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/exception.h"
#include "app/icons.h"
#include "app/logging.h"
#include "portintask.h"
#include "os/conn.h"
#include "lib/protocol.h"
#include "lib/wire.h"

#include <QSocketNotifier>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#define TR_TASKOBJ1 TL("task-object", "This computer")
#define TR_TASKTYPE1 TL("task-type", "Remote Port Forwarding")

#define BACKLOG 100

PortInTask::PortInTask(ServerInstance *server, const PortFwdRule &config) :
    PortFwdTask(server, config)
{
    m_typeStr = TR_TASKTYPE1;
    m_typeIcon = ICON_TASKTYPE_PORT_FORWARD_IN;
    m_toStr = TR_TASKOBJ1;
    m_sourceStr = config.remoteStr();
    m_sinkStr = config.localStr();
}

int
PortInTask::connectHelper(PortFwdState &state, struct addrinfo *p)
{
    int fd = -1;

    for (; p; p = p->ai_next) {
        logDebug(lcCommand, "PortFwd %p: %u: connecting to %p", this, state.id, p);

        fd = osSocket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1) {
            logDebug(lcCommand, "PortFwd %p: %u: socket: %s", this, state.id,
                     strerror(errno));
            continue;
        }
        if (::connect(fd, p->ai_addr, p->ai_addrlen) < 0) {
            if (errno != EINPROGRESS) {
                logDebug(lcCommand, "PortFwd %p: %u: connect: %s", this, state.id,
                         strerror(errno));
                close(fd);
                fd = -1;
                continue;
            }

            // Set up deferred connect
            logDebug(lcCommand, "PortFwd %p: %u: connection deferred", this, state.id);
            state.p = p;
            state.special = true;
            break;
        }

        // Have connection now
        logDebug(lcCommand, "PortFwd %p: %u: connected", this, state.id);
        state.special = false;
        break;
    }

    return fd;
}

int
PortInTask::connectTcp(PortFwdState &state)
{
    struct addrinfo hints, *a;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;

    int rc = getaddrinfo(m_config.laddr.data(), m_config.lport.data(), &hints, &a);
    if (rc < 0) {
        logDebug(lcCommand, "PortFwd %p: %u: getaddrinfo: %s", this, state.id,
                 gai_strerror(rc));
        return -1;
    }

#ifndef NDEBUG
    for (const auto *p = a; p; p = p->ai_next) {
        char buf[256], port[256];
        int rc = getnameinfo(p->ai_addr, p->ai_addrlen, buf, sizeof(buf),
                             port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV);
        logDebug(lcCommand, "PortFwd %p: %u: connect candidate %p: "
                 "%s:%s, family %d, socktype %d, protocol %d",
                 this, state.id, p, rc ? "?" : buf, rc ? "?" : port,
                 p->ai_family, p->ai_socktype, p->ai_protocol);
    }
#endif

    state.p = state.a = a;
    return connectHelper(state, a);
}

int
PortInTask::connectUnix(PortFwdState &state)
{
    int fd;

    try {
        fd = osLocalConnect(m_config.laddr.c_str());
    } catch (const ErrnoException &) {
        fd = -1;
    }

    if (fd == -1) {
        logDebug(lcCommand, "PortFwd %p: %u: %s: error", this, state.id, m_config.laddr.c_str());
    } else {
        state.special = false;
    }

    return fd;
}

void
PortInTask::handleStart(Tsq::ProtocolUnmarshaler *unm)
{
    portfwd_t id = unm->parseNumber();

    if (m_idmap.count(id)) {
        return;
    }

    int cfd;
    PortFwdState *cstate = new PortFwdState{ id };

    if (m_config.ltype == Tsq::PortForwardTCP)
        cfd = connectTcp(*cstate);
    else
        cfd = connectUnix(*cstate);

    if (cfd >= 0) {
        cstate->fd = cfd;
        cstate->writer = new QSocketNotifier(cfd, QSocketNotifier::Write);
        cstate->caddr = unm->parseString();
        cstate->cport = unm->parseString();

        if (cstate->special) {
            connect(cstate->writer, SIGNAL(activated(int)), SLOT(handleConnect(int)));
        } else {
            cstate->reader = new QSocketNotifier(cfd, QSocketNotifier::Read);
            connect(cstate->reader, SIGNAL(activated(int)), SLOT(handleRead(int)));
            connect(cstate->writer, SIGNAL(activated(int)), SLOT(handleWrite(int)));
            cstate->writer->setEnabled(false);
        }

        m_fdmap[cfd] = cstate;
        m_idmap[id] = cstate;
        emit connectionAdded(cstate->id);
    }
    else {
        delete cstate;
        pushBytes(id, 0);
    }
}

void
PortInTask::handleConnect(int fd)
{
    auto *cstate = m_fdmap[fd];
    portfwd_t id = cstate->id;

    // Handle deferred connection
    socklen_t socklen = sizeof(int);
    int sockerr;
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &sockerr, &socklen) < 0)
        sockerr = errno;

    if (sockerr) {
        logDebug(lcCommand, "PortFwd %p: %u: connect (deferred): %s", this, id,
                 strerror(sockerr));

        int newfd = connectHelper(*cstate, cstate->p->ai_next);
        if (newfd < 0) {
            logDebug(lcCommand, "PortFwd %p: %u: failed to connect to any address", this, id);
            closefd(id);
            pushBytes(id, 0);
            return;
        }

        m_fdmap.erase(fd);
        close(fd);
        cstate->fd = newfd;
        m_fdmap[newfd] = cstate;
    }
    else {
        logDebug(lcCommand, "PortFwd %p: %u: connected (deferred)", this, id);
        cstate->special = false;
    }

    delete cstate->writer;
    cstate->writer = new QSocketNotifier(cstate->fd, QSocketNotifier::Write);

    if (cstate->special) {
        connect(cstate->writer, SIGNAL(activated(int)), SLOT(handleConnect(int)));
    } else {
        cstate->reader = new QSocketNotifier(cstate->fd, QSocketNotifier::Read);
        connect(cstate->reader, SIGNAL(activated(int)), SLOT(handleRead(int)));
        connect(cstate->writer, SIGNAL(activated(int)), SLOT(handleWrite(int)));
        cstate->writer->setEnabled(!cstate->outdata.empty());
    }
}

void
PortInTask::start(TermManager *manager)
{
    PortFwdTask::start(manager, TSQ_LISTENING_PORTFWD);
}

TermTask *
PortInTask::clone() const
{
    return new PortInTask(m_server, m_config);
}
