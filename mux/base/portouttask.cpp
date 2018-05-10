// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "portouttask.h"
#include "listener.h"
#include "exception.h"
#include "os/conn.h"
#include "os/logging.h"
#include "lib/wire.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

PortOut::PortOut(Tsq::ProtocolUnmarshaler *unm) :
    PortBase("portout", unm, 0)
{
}

int
PortOut::connectHelper(PortFwdState &state, short &eventret, struct addrinfo *p)
{
    int fd = -1;

    for (; p; p = p->ai_next) {
        LOGDBG("PortFwd %p: %u: connecting to %p\n", this, state.id, p);

        fd = osSocket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1) {
            LOGDBG("PortFwd %p: %u: socket: %m\n", this, state.id);
            continue;
        }
        if (connect(fd, p->ai_addr, p->ai_addrlen) < 0) {
            if (errno != EINPROGRESS) {
                LOGDBG("PortFwd %p: %u: connect: %m\n", this, state.id);
                close(fd);
                fd = -1;
                continue;
            }

            // Set up deferred connect
            LOGDBG("PortFwd %p: %u: connection deferred\n", this, state.id);
            state.p = p;
            state.special = true;
            eventret = POLLOUT;
            break;
        }

        // Have connection now
        LOGDBG("PortFwd %p: %u: connected\n", this, state.id);
        state.special = false;
        eventret = state.outdata.empty() ? POLLIN : POLLIN|POLLOUT;
        break;
    }

    return fd;
}

int
PortOut::connectTcp(PortFwdState &state, short &eventret)
{
    struct addrinfo hints, *a;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;

    int rc = getaddrinfo(m_address.c_str(), m_port.c_str(), &hints, &a);
    if (rc < 0) {
        LOGDBG("PortFwd %p: %u: getaddrinfo: %s\n", this, state.id, gai_strerror(rc));
        return -1;
    }

#ifndef NDEBUG
    for (const auto *p = a; p; p = p->ai_next) {
        char buf[256], port[256];
        int rc = getnameinfo(p->ai_addr, p->ai_addrlen, buf, sizeof(buf),
                             port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV);
        LOGDBG("PortFwd %p: %u: connect candidate %p: "
               "%s:%s, family %d, socktype %d, protocol %d\n",
               this, state.id, p, rc ? "?" : buf, rc ? "?" : port,
               p->ai_family, p->ai_socktype, p->ai_protocol);
    }
#endif

    state.p = state.a = a;
    return connectHelper(state, eventret, a);
}

int
PortOut::connectUnix(PortFwdState &state, short &eventret)
{
    int fd;

    try {
        fd = osLocalConnect(m_address.c_str());
    } catch (const ErrnoException &) {
        fd = -1;
    }

    if (fd == -1) {
        LOGDBG("PortFwd %p: %u: %s: %m\n", this, state.id, m_address.c_str());
    } else {
        eventret = POLLIN;
    }

    return fd;
}

inline void
PortOut::connectfd(pollfd &pfd, PortFwdState *cstate)
{
    portfwd_t id = cstate->id;

    // Handle deferred connection
    socklen_t socklen = sizeof(int);
    int sockerr;
    if (getsockopt(pfd.fd, SOL_SOCKET, SO_ERROR, &sockerr, &socklen) < 0)
        sockerr = errno;

    if (sockerr) {
        LOGDBG("PortFwd %p: %u: connect (deferred): %s\n", this, id, strerror(sockerr));

        int newfd = connectHelper(*cstate, pfd.events, cstate->p->ai_next);
        if (newfd < 0) {
            LOGDBG("PortFwd %p: %u: failed to connect to any address\n", this, id);
            closefd(id);
            pushBytes(id, 0);
        } else {
            m_fdmap.erase(pfd.fd);
            close(pfd.fd);
            pfd.fd = cstate->fd = newfd;
            m_fdmap[newfd] = cstate;
        }
    }
    else {
        LOGDBG("PortFwd %p: %u: connected (deferred)\n", this, id);
        cstate->special = false;
        pfd.events = cstate->outdata.empty() ? POLLIN : POLLIN|POLLOUT;
    }
}

void
PortOut::handleStart(portfwd_t id)
{
    if (m_idmap.count(id)) {
        LOGDBG("PortFwd %p: ignoring existing id %u\n", this, id);
        return;
    }

    int cfd;
    short events;
    PortFwdState *cstate = new PortFwdState{ id };

    if (m_type == Tsq::PortForwardTCP)
        cfd = connectTcp(*cstate, events);
    else
        cfd = connectUnix(*cstate, events);

    if (cfd >= 0) {
        cstate->fd = cfd;
        pollfd pfd = { .fd = cfd, .events = events };

        m_fds.push_back(pfd);
        m_fdmap[cfd] = cstate;
        m_idmap[id] = cstate;
    }
    else {
        LOGDBG("PortFwd %p: %u: failed to connect to any address\n", this, id);
        delete cstate;
        pushBytes(id, 0);
    }
}

bool
PortOut::handleMultiFd(pollfd &pfd)
{
    auto *cstate = m_fdmap[pfd.fd];

    if (cstate->special)
        connectfd(pfd, cstate);
    else if (pfd.revents & POLLOUT)
        writefd(pfd, cstate);
    else if (m_sent - m_acked >= m_windowSize * m_chunkSize)
        watchReads(m_running = false);
    else
        readfd(pfd, cstate);

    return true;
}

void
PortOut::threadMain()
{
    try {
        bool ok;

        switch (m_type) {
        case Tsq::PortForwardTCP:
            LOGDBG("PortFwd %p: to tcp:%s:%s\n", this, m_address.c_str(), m_port.c_str());
            ok = true;
            break;
        case Tsq::PortForwardUNIX:
            LOGDBG("PortFwd %p: to unix:%s\n", this, m_address.c_str());
            ok = true;
            break;
        default:
            LOGERR("PortFwd %p: invalid type code %u\n", this, m_type);
            ok = false;
            break;
        }

        if (ok) {
            runDescriptorLoopMulti();
        }
    }
    catch (const std::exception &e) {
        LOGERR("PortFwd %p: caught exception: %s\n", this, e.what());
    }

    for (const auto &i: m_fdmap) {
        close(i.first);
        freeaddrinfo(i.second->a);
        delete i.second;
    }

    g_listener->sendWork(ListenerRemoveTask, this);
}
