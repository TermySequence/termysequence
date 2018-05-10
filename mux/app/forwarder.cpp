// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "forwarder.h"
#include "args.h"
#include "base/exception.h"
#include "os/conn.h"
#include "os/signal.h"
#include "os/pty.h"
#include "lib/protocol.h"
#include "lib/sequences.h"
#include "lib/exitcode.h"
#include "config.h"

#include <sys/socket.h>
#include <cstdio>
#include <unistd.h>

enum ForwarderState {
    ReadingHandshake, WritingHandshake, ReadingResponse,
    WritingDescriptors, ReadingInfo
};

/*
 * Forwarder
 */
TermForwarder::TermForwarder() :
    ThreadBase("forwarder", ThreadBaseFd),
    m_protocolType(TSQ_PROTOCOL_REJECT),
    m_state(ReadingHandshake)
{
}

TermForwarder::~TermForwarder()
{
    delete m_writer;
    delete m_reader;
    delete [] m_termios;
}

bool
TermForwarder::handleInterrupt()
{
    m_exitStatus = TSQ_STATUS_FORWARDER_SHUTDOWN;
    return false;
}

bool
TermForwarder::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case ForwarderReaderExit:
        return false;
    case ForwarderWriterExit:
        m_writerExit = true;
        return !m_cleanExit;
    default:
        m_exitStatus = item.value;
        return false;
    }
}

void
TermForwarder::readHandshake()
{
    char buf[256];
    ssize_t rc = read(m_fd, buf, sizeof(buf));
    if (rc < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        if (errno == EINTR)
            return;

        throw ErrnoException("Read failed during handshake", errno);
    }
    if (rc == 0) {
        throw TsqException("Received EOF during handshake");
    }

    rc = m_handshake.processHello(buf, rc);
    if (rc == Tsq::ShakeOngoing)
        return;
    if (rc != Tsq::ShakeSuccess)
        throw ProtocolException(); // Received bad handshake message from server
    if (m_handshake.protocolVersion != TSQ_PROTOCOL_VERSION)
        throw TsqException("Unsupported server protocol version %u", m_handshake.protocolVersion);

    m_outbuf = m_handshake.getResponse(TSQ_PROTOCOL_VERSION, TSQ_PROTOCOL_CLIENTFD);
    m_fds[1].events = POLLOUT;
    m_state = WritingHandshake;
}

void
TermForwarder::writeHandshake()
{
    ssize_t rc = write(m_fd, m_outbuf.data(), m_outbuf.size());
    if (rc < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        if (errno == EINTR)
            return;

        throw ErrnoException("Write failed during handshake", errno);
    }

    m_outbuf.erase(0, rc);

    if (m_outbuf.empty()) {
        m_fds[1].events = POLLIN;
        m_state = ReadingResponse;
    }
}

void
TermForwarder::readResponse()
{
    char c;

    ssize_t rc = read(m_fd, &c, 1);
    if (rc < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        if (errno == EINTR)
            return;

        throw ErrnoException("Read failed during handshake", errno);
    }
    if (rc == 0)
        throw TsqException("Received EOF during handshake");
    if (c)
        throw TsqException("Received bad response message from server");

    if (m_needsForwarding)
        osSocketPair(m_sd, true);

    m_fds[1].events = POLLOUT;
    m_state = WritingDescriptors;
}

void
TermForwarder::writeDescriptors()
{
    int payload[2];

    if (m_needsForwarding) {
        payload[1] = payload[0] = m_sd[1];
    } else {
        payload[0] = STDIN_FILENO;
        payload[1] = STDOUT_FILENO;
    }

    if (osLocalSendFd(m_fd, payload, 2) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        if (errno == EINTR)
            return;

        throw ErrnoException("Send of file descriptor pair failed", errno);
    }

    if (m_needsForwarding) {
        // Launch forwarder
        if (tcgetpgrp(STDIN_FILENO) != getpgrp())
            throw TsqException("Cannot run in the background");

        osMakeSignalTerminal(STDIN_FILENO, &m_termios);

        close(m_sd[1]);
        m_reader = new TermForwarderWriter(this, STDIN_FILENO, m_sd[0],
                                           ForwarderReaderExit);
        m_writer = new TermForwarderWriter(this, m_sd[0], STDOUT_FILENO,
                                           ForwarderWriterExit);
        m_writer->start(-1);
        m_reader->start(-1);
    }

    m_fds[1].events = POLLIN;
    m_state = ReadingInfo;
}

bool
TermForwarder::readInfo()
{
    char c;
    ssize_t rc = read(m_fd, &c, 1);

    if (rc < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;
        if (errno == EINTR)
            return true;

        m_exitStatus = TSQ_STATUS_LOST_CONN;
        return false;
    }
    else if (rc == 0) {
        m_exitStatus = TSQ_STATUS_LOST_CONN;
        return false;
    }
    else if (c >= 0) {
        // Read the exit status
        m_exitStatus = c;
        m_cleanExit = true;

        if (m_needsForwarding) {
            m_fds[1].fd = -1;
            return !m_writerExit;
        }
        return false;
    }
    else {
        // Read the negotiated protocol type
        m_protocolType = -c;
        return true;
    }
}

bool
TermForwarder::handleMultiFd(pollfd &)
{
    switch (m_state) {
    case ReadingHandshake:
        readHandshake();
        return true;
    case WritingHandshake:
        writeHandshake();
        return true;
    case ReadingResponse:
        readResponse();
        return true;
    case WritingDescriptors:
        writeDescriptors();
        return true;
    default:
        return readInfo();
    }
}

inline void
TermForwarder::writeDisconnect()
{
    osIgnoreBackgroundSignals();
    osWaitForWritable(STDOUT_FILENO);

    switch(m_protocolType) {
    case TSQ_PROTOCOL_RAW:
        write(STDOUT_FILENO, TSQ_RAW_DISCONNECT, TSQ_RAW_DISCONNECT_LEN);
        break;
    case TSQ_PROTOCOL_TERM:
        write(STDOUT_FILENO, TSQ_TERM_DISCONNECT, TSQ_TERM_DISCONNECT_LEN);
        break;
    }
}

void
TermForwarder::threadMain()
{
    osMakeNonblocking(m_fd);
    osMakeNonblocking(STDIN_FILENO);
    osMakeNonblocking(STDOUT_FILENO);
    m_needsForwarding = isatty(STDIN_FILENO);
    loadfd();

    try {
        runDescriptorLoopMulti();
    }
    catch (const std::exception &e) {
        fprintf(stderr, "%s\n", e.what());
        m_exitStatus = TSQ_STATUS_FORWARDER_ERROR;
    }

    if (m_reader) {
        m_reader->stop(0);
        m_writer->stop(0);
        m_reader->join();
        m_writer->join();

        osRestoreTerminal(STDIN_FILENO, m_termios);
    }

    if (!m_cleanExit)
        writeDisconnect();

    m_fd = -1;
}

/*
 * ForwarderWriter
 */
TermForwarderWriter::TermForwarderWriter(TermForwarder *parent, int infd,
                                         int outfd, int code) :
    ThreadBase("fwdwriter", ThreadBaseMulti),
    m_infd(infd),
    m_outfd(outfd),
    m_parent(parent),
    m_code(code)
{
    m_fds.emplace_back(pollfd{ .fd = infd, .events = POLLIN });
    m_fds.emplace_back(pollfd{ .fd = outfd });

    m_buf = new char[READER_BUFSIZE];
}

TermForwarderWriter::~TermForwarderWriter()
{
    delete [] m_buf;
}

bool
TermForwarderWriter::handleMultiFd(pollfd &)
{
    ssize_t rc;
    char *ptr;
    size_t total;

    if (!m_ptr) {
        rc = read(m_infd, m_buf, READER_BUFSIZE);
        if (rc < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return true;
            if (errno == EINTR)
                return true;

            m_parent->stop(TSQ_STATUS_FORWARDER_ERROR);
            return false;
        }
        if (rc == 0) {
            if (m_infd == STDIN_FILENO)
                shutdown(m_outfd, SHUT_WR);

            m_parent->sendWork(m_code, 0);
            return false;
        }

        ptr = m_buf;
        total = rc;
    } else {
        ptr = m_ptr;
        total = m_ptrlen;
    }

    while (total) {
        rc = write(m_outfd, ptr, total);
        if (rc < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            if (errno == EINTR)
                break;

            m_parent->stop(TSQ_STATUS_FORWARDER_ERROR);
            return false;
        }

        total -= rc;
        ptr += rc;
    }

    if (total) {
        m_ptr = ptr;
        m_ptrlen = total;
        m_fds[1].events = 0;
        m_fds[2].events = POLLOUT;
    } else if (m_ptr) {
        m_ptr = nullptr;
        m_fds[1].events = POLLIN;
        m_fds[2].events = 0;
    }

    return true;
}

void
TermForwarderWriter::threadMain()
{
    try {
        runDescriptorLoopMulti();
    }
    catch (const std::exception &) {
        m_parent->stop(TSQ_STATUS_FORWARDER_ERROR);
    }
}

void
runForwarder(int fd)
{
    g_args->renameProcess(FORWARDER_NAME);

    TermForwarder forwarder;
    int rc = forwarder.run(fd);
    int status = g_args->printExitStatus(rc);

    exit(status);
}
