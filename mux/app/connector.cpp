// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "connector.h"
#include "args.h"
#include "base/exception.h"
#include "base/status.h"
#include "os/process.h"
#include "os/pty.h"
#include "os/conn.h"
#include "os/dir.h"
#include "os/fd.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/exitcode.h"
#include "lib/attr.h"
#include "lib/attrstr.h"
#include "lib/trstr.h"
#include "config.h"

#include <cstdio>
#include <unistd.h>

TermConnector::TermConnector(int pid, bool pty, bool raw) :
    ThreadBase("connector", ThreadBaseMulti)
{
    m_pid = pid;
    m_protocolType = raw ? TSQ_PROTOCOL_RAW_SERVERFD : TSQ_PROTOCOL_TERM_SERVERFD;
    m_pty = pty;
    m_fdp = &m_fds;
    m_exitStatusPtr = &m_exitStatus;
}

TermConnector::~TermConnector()
{
    if (m_termios) {
        osRestoreTerminal(STDIN_FILENO, m_termios);
        delete [] m_termios;
    }
}

bool
TermConnector::localConnect()
{
    // Place terminal in raw mode
    if (m_pty)
        osMakeRawTerminal(m_fd);

    // Collect process attributes
    TermStatusTracker status;
    status.updateOnce(m_fd, m_pid, CONNECT_NAME);
    m_attributes.append(4, '\0');

    for (const auto &i: status.changedMap()) {
        setAttribute(i.first, i.second);
    }

    // Try to collect initiator attribute from terminal
    if (m_termios && g_args->query()) {
        std::string sender;
        osMakeRawTerminal(STDIN_FILENO);
        bool rc = osGetEmulatorAttribute(TSQ_ATTR_SENDER_ID, sender);
        osRestoreTerminal(STDIN_FILENO, m_termios);
        if (rc)
            setAttribute(Tsq::attr_SENDER_ID, sender);
    }

    // Finish out attributes
    unsigned keepalive = g_args->keepalive();
    if (keepalive) {
        setAttribute(Tsq::attr_COMMAND_KEEPALIVE, std::to_string(keepalive));
    }
    uint32_t len = htole32(m_attributes.size() - 4);
    m_attributes.replace(0, 4, reinterpret_cast<char*>(&len), 4);

    // Make local connection
    m_sd = osServerConnect(g_args->rundir());
    if (m_sd == -1) {
        setError(Tsq::ConnectTaskErrorLocalConnectFailed);
        return false;
    }

    m_handshake.reset(true);
    m_state = ReadingLocalHandshake;
    m_fds.pop_back();
    m_fds[1].fd = m_sd;
    return true;
}

bool
TermConnector::readRemoteHandshake(pollfd &pfd)
{
    char buf[1024];

    if (pfd.fd == STDIN_FILENO) {
        ssize_t rc = read(STDIN_FILENO, buf, sizeof(buf));
        if (rc <= 0)
            return setHandshakeReadError(rc);

        if (m_readCount >= CONNECT_HANDSHAKE_MAX) {
            setError(Tsq::ConnectTaskErrorRemoteLimitExceeded);
            return false;
        }

        m_readCount += rc;
        m_outbuf.assign(buf, rc);

        // Show enter presses
        while (rc--)
            if (buf[rc] == '\n')
                putchar('\n');

        m_state = WritingRemoteHandshake;
        m_fds[1].events = POLLOUT;
        m_fds[2].fd = -1;
    }
    else {
        ssize_t rc = read(m_fd, buf, sizeof(buf));
        if (rc <= 0)
            return setHandshakeReadError(rc);

        for (size_t i = 0; i < rc; ++i)
        {
            int rc2 = m_handshake.processHelloByte(buf[i]);
            switch (rc2) {
            case Tsq::ShakeOngoing:
                continue;
            case Tsq::ShakeSuccess:
                m_hello = m_handshake.getHello();
                return localConnect();
            case Tsq::ShakeBadLeadingContent:
                m_outbuf.append(m_handshake.leadingContent, m_handshake.leadingContentLen);
                continue;
            default:
                setError(Tsq::ConnectTaskErrorRemoteHandshakeFailed, true, rc2);
                return false;
            }
        }
        if (!m_outbuf.empty()) {
            m_state = WritingRemoteHandshake;
            m_fds[1].fd = -1;
            m_fds[2].events = POLLOUT;
        }
    }
    return true;
}

bool
TermConnector::writeRemoteHandshake(pollfd &pfd)
{
    switch (writeData(pfd.fd, m_outbuf)) {
    case 1:
        m_state = ReadingRemoteHandshake;
        m_fds[1] = pollfd{ .fd = m_fd, .events = POLLIN };
        m_fds[2] = pollfd{ .fd = STDIN_FILENO, .events = POLLIN };
        // fallthru
    case 0:
        return true;
    default:
        return false;
    }
}

bool
TermConnector::handleMultiFd(pollfd &pfd)
{
    return processConnectorFd(pfd, m_fd);
}

void
TermConnector::threadMain()
{
    osMakeNonblocking(STDIN_FILENO);
    osMakeNonblocking(STDOUT_FILENO);

    m_fds.emplace_back(pollfd{ .fd = m_fd, .events = POLLIN });
    m_fds.emplace_back(pollfd{ .fd = STDIN_FILENO, .events = POLLIN });

    try {
        if (isatty(STDIN_FILENO))
            osMakePasswordTerminal(STDIN_FILENO, &m_termios);

        runDescriptorLoopMulti();

        if (m_exitStatus)
            m_exitStatus = g_args->printConnectError(m_errtype, m_errnum, errno);
    }
    catch (const std::exception &e) {
        fprintf(stderr, "%s\n", e.what());
        m_exitStatus = EXITCODE_CONNECTERR;
    }

    m_fd = -1;
}

int
runConnector()
{
    PtyParams params{};
    bool pty = g_args->pty();
    bool raw = g_args->raw();
    int fd, pid;

    if (g_args->arg0().empty()) {
        params.command.append(g_args->cmd().c_str());
        params.command.push_back('\0');
        params.command.append(g_args->cmd());
    } else {
        params.command.append(g_args->arg0().c_str());
        params.command.push_back('\0');
        params.command.append(g_args->cmd());
    }

    params.env = g_args->env();
    if (!params.env.empty())
        params.env.push_back('\0');

    params.dir = g_args->dir();
    osRelativeToHome(params.dir);
    params.daemon = true;

    osPurgeFileDescriptors(CONNECT_NAME);

    if (pty) {
        params.env.append("+TERM=dumb");
        fd = osForkTerminal(params, &pid);
    } else {
        params.env.append("-TERM");
        fd = osForkProcess(params, &pid);
    }

    TermConnector connector(pid, pty, raw);
    return connector.run(fd);
}
