// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/exception.h"
#include "app/icons.h"
#include "app/logging.h"
#include "portouttask.h"
#include "os/conn.h"
#include "lib/protocol.h"

#include <QSocketNotifier>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#define TR_TASKOBJ1 TL("task-object", "This computer")
#define TR_TASKTYPE1 TL("task-type", "Local Port Forwarding")

#define BACKLOG 100

PortOutTask::PortOutTask(ServerInstance *server, const PortFwdRule &config) :
    PortFwdTask(server, config)
{
    m_typeStr = TR_TASKTYPE1;
    m_typeIcon = ICON_TASKTYPE_PORT_FORWARD_OUT;
    m_fromStr = TR_TASKOBJ1;
    m_sourceStr = config.localStr();
    m_sinkStr = config.remoteStr();
}

bool
PortOutTask::listenTcp(TermManager *manager)
{
    struct addrinfo hints, *a, *p;
    const char *node = !m_config.laddr.empty() ? m_config.laddr.c_str() : nullptr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE|AI_ADDRCONFIG;

    int rc = getaddrinfo(node, m_config.lport.c_str(), &hints, &a);
    if (rc < 0) {
        failStart(manager, gai_strerror(rc));
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
            close(fd);
            continue;
        }
        if (listen(fd, BACKLOG) < 0) {
            close(fd);
            continue;
        }

        PortFwdState *lstate = new PortFwdState{ INVALID_PORTFWD, fd };

        lstate->reader = new QSocketNotifier(fd, QSocketNotifier::Read);
        connect(lstate->reader, SIGNAL(activated(int)), SLOT(handleAccept(int)));
        m_fdmap[fd] = lstate;
    }

    freeaddrinfo(a);
    if (m_fdmap.empty()) {
        failStart(manager, strerror(EADDRNOTAVAIL));
        return false;
    }
    return true;
}

bool
PortOutTask::listenUnix(TermManager *manager)
{
    return false;
}

void
PortOutTask::start(TermManager *manager)
{
    if (m_config.ltype == Tsq::PortForwardTCP && !listenTcp(manager))
        return;
    if (m_config.ltype == Tsq::PortForwardUNIX && !listenUnix(manager))
        return;

    PortFwdTask::start(manager, TSQ_CONNECTING_PORTFWD);
}

TermTask *
PortOutTask::clone() const
{
    return new PortOutTask(m_server, m_config);
}
