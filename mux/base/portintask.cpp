// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "portintask.h"
#include "listener.h"
#include "exception.h"
#include "os/conn.h"
#include "os/user.h"
#include "os/logging.h"
#include "lib/protocol.h"
#include "lib/wire.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#define BACKLOG 100

PortIn::PortIn(Tsq::ProtocolUnmarshaler *unm) :
    PortBase("portin", unm, TaskBaseExclusive)
{
    switch (m_type) {
    case Tsq::PortForwardTCP:
        m_targetName = m_address;
        m_targetName.push_back(':');
        m_targetName.append(m_port);
        break;
    case Tsq::PortForwardUNIX:
        m_targetName = m_address;
        break;
    default:
        break;
    }
}

bool
PortIn::listenTcp()
{
    struct addrinfo hints, *a, *p;
    const char *node = !m_address.empty() ? m_address.c_str() : nullptr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE|AI_ADDRCONFIG;

    int rc = getaddrinfo(node, m_port.c_str(), &hints, &a);
    if (rc < 0) {
        reportError(Tsq::PortForwardTaskErrorBadAddress, gai_strerror(rc));
        LOGDBG("PortFwd %p: getaddrinfo: %s\n", this, gai_strerror(rc));
        return false;
    }

    rc = 1;
    for (p = a; p; p = p->ai_next) {
        int fd = osSocket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) {
            continue;
        }
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &rc, sizeof(rc)) < 0) {
            close(fd);
            continue;
        }
        if (bind(fd, p->ai_addr, p->ai_addrlen) < 0) {
            LOGDBG("PortFwd %p: bind: %m\n", this);
            close(fd);
            continue;
        }
        if (listen(fd, BACKLOG) < 0) {
            close(fd);
            continue;
        }

        pollfd pfd = { .fd = fd, .events = POLLIN };
        m_fds.push_back(pfd);

        PortFwdState *lstate = new PortFwdState{ INVALID_PORTFWD, fd };
        lstate->special = true;
        m_fdmap[fd] = lstate;
    }

    freeaddrinfo(a);
    if (m_fds.size() == 1) {
        // Fail
        reportError(Tsq::PortForwardTaskErrorBindFailed, strerror(EADDRNOTAVAIL));
        LOGNOT("PortFwd %p: failed to bind to any address\n", this);
        return false;
    }
    return true;
}

bool
PortIn::listenUnix()
{
    int fd;

    try {
        fd = osLocalListen(m_address.c_str(), BACKLOG);
    } catch (const ErrnoException &) {
        fd = -1;
    }

    if (fd >= 0) {
        pollfd pfd = { .fd = fd, .events = POLLIN };
        m_fds.push_back(pfd);

        PortFwdState *lstate = new PortFwdState{ INVALID_PORTFWD, fd };
        lstate->special = true;
        m_fdmap[fd] = lstate;
        return true;
    } else {
        LOGDBG("PortFwd %p: %s: %m\n", this, m_address.c_str());
        return false;
    }
}

inline void
PortIn::acceptfd(pollfd &pfd, PortFwdState *state)
{
    int cfd;
    char buf[512];
    struct sockaddr *ptr = (struct sockaddr *)buf;
    socklen_t len = sizeof(buf);

    try {
        cfd = osAccept(pfd.fd, ptr, &len);
    }
    catch (const Tsq::ErrnoException &) {
        LOGNOT("PortFwd %p: accept failed: %m\n", this);
        cfd = -1;

        // Close this listener
        m_fdmap.erase(pfd.fd);
        close(pfd.fd);
        pfd.fd = -1;
        delete state;
    }

    if (cfd == -1)
        return;
    ++m_nextId;
    m_nextId += (m_nextId == INVALID_PORTFWD);

    LOGDBG("PortFwd %p: %u: accepted\n", this, m_nextId);
    pollfd newfd = { .fd = cfd, .events = POLLIN };
    m_fds.push_back(newfd);

    PortFwdState *cstate = new PortFwdState{ m_nextId, cfd };
    m_fdmap[cfd] = cstate;
    m_idmap[m_nextId] = cstate;

    if (m_type == Tsq::PortForwardTCP) {
        char host[64], serv[32];
        if (getnameinfo(ptr, len, host, 64, serv, 32, NI_NUMERICHOST|NI_NUMERICSERV))
            host[0] = serv[0] = '\0';

        pushStart(m_nextId, host, serv);
    }
    else {
        int uid, pid;
        std::string str;
        if (osLocalCreds(cfd, uid, pid)) {
            str = osUserName(uid);
            str.push_back(':');
            str.append(pid != -1 ? std::to_string(pid) : "?");
        }
        pushStart(m_nextId, "", str.c_str());
    }
}

bool
PortIn::handleMultiFd(pollfd &pfd)
{
    auto *cstate = m_fdmap[pfd.fd];

    if (pfd.revents & POLLOUT)
        writefd(pfd, cstate);
    else if (m_sent - m_acked >= m_windowSize * m_chunkSize)
        watchReads(m_running = false);
    else if (cstate->special)
        acceptfd(pfd, cstate);
    else
        readfd(pfd, cstate);

    return true;
}

void
PortIn::threadMain()
{
    try {
        bool ok;

        switch (m_type) {
        case Tsq::PortForwardTCP:
            LOGDBG("PortFwd %p: from tcp:%s:%s\n", this, m_address.c_str(), m_port.c_str());
            ok = listenTcp();
            break;
        case Tsq::PortForwardUNIX:
            LOGDBG("PortFwd %p: from unix:%s\n", this, m_address.c_str());
            ok = listenUnix();
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
        delete i.second;
    }

    g_listener->sendWork(ListenerRemoveTask, this);
}
