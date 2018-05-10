// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "app/protocol.h"
#include "conn.h"
#include "args.h"
#include "lib/exception.h"
#include "lib/wire.h"
#include "lib/raw.h"

#include <unistd.h>

#define TR_TEXT1 TL("cli", "Writing to %1 on server %2")
#define TR_TEXT2 TL("cli", "Reading from %1 on server %2")

CliConnection::CliConnection(int fd, const ArgParser *args) :
    m_args(args),
    m_fd(fd)
{
    m_machine = new Tsq::RawProtocol(this);
}

CliConnection::~CliConnection()
{
    delete m_machine;
}

void
CliConnection::doForward(int ifd, int ofd, const char *rbuf, size_t rlen)
{
    m_fd = ofd;
    writeFd(rbuf, rlen);

    char buf[READER_BUFSIZE];
    while (true) {
        ssize_t rc = read(ifd, buf, sizeof(buf));
        switch (rc) {
        default:
            writeFd(buf, rc);
            break;
        case 0:
            return;
        case -1:
            throw Tsq::ErrnoException("read", errno);
        }
    }
}

bool
CliConnection::run()
{
    m_response = 0;
    Tsq::ProtocolMarshaler m(m_args->cmd());

    switch (m_args->cmd()) {
    case TSQT_UPLOAD_PIPE:
    case TSQT_DOWNLOAD_PIPE:
        m.addString(*m_args->args());
        break;
    case TSQT_RUN_ACTION:
        m.addString(m_args->args()[m_iter++]);
        break;
    }

    m_machine->connFlush(m.resultPtr(), m.length());

    ssize_t got;
    size_t done;
    char buf[4096];

    while (true) {
        switch (got = read(m_fd, buf, sizeof(buf))) {
        default:
            // feed the machine, repeatedly
            done = 0;
            while (done < got) {
                done += m_machine->connProcess(buf + done, got - done);
                if (m_response)
                    goto finished;
            }
            break;
        case 0:
            errno = ECONNABORTED;
            // fallthru
        case -1:
            if (errno == EINTR || errno == EAGAIN)
                break;

            throw Tsq::ErrnoException("read", errno);
        }
    }

finished:
    switch (m_response) {
    case TSQT_UPLOAD_PIPE:
        doForward(STDIN_FILENO, m_fd, nullptr, 0);
        return false;
    case TSQT_DOWNLOAD_PIPE:
        doForward(m_fd, STDOUT_FILENO, buf + done, got - done);
        return false;
    case TSQT_RUN_ACTION:
        return m_iter < m_args->argc();
    default:
        return false;
    }
}

bool
CliConnection::protocolCallback(uint32_t command, uint32_t length, const char *body)
{
    Tsq::ProtocolUnmarshaler unm(body, length);
    unsigned code;
    std::string str;

    if (command != m_args->cmd())
        throw Tsq::ErrnoException(EINVAL);

    switch (command) {
    case TSQT_UPLOAD_PIPE:
        code = unm.parseNumber();
        str = unm.parseString();
        if (code != 0)
            throw Tsq::TsqException(str.c_str());
        fprintf(stderr, "%s\n", pr(TR_TEXT1.arg(str.c_str(), *m_args->args())));
        break;
    case TSQT_DOWNLOAD_PIPE:
        code = unm.parseNumber();
        str = unm.parseString();
        if (code != 0)
            throw Tsq::TsqException(str.c_str());
        fprintf(stderr, "%s\n", pr(TR_TEXT2.arg(str.c_str(), *m_args->args())));
        break;
    case TSQT_LIST_SERVERS:
        while (!(str = unm.parseString()).empty())
            printf("%s\n", str.c_str());
        break;
    case TSQT_RUN_ACTION:
        code = unm.parseNumber();
        if (code != 0)
            throw Tsq::TsqException(unm.parseString().c_str());
        break;
    }

    m_response = command;
    return true;
}

void
CliConnection::writeFd(const char *buf, size_t len)
{
    while (len) {
        ssize_t rc = write(m_fd, buf, len);
        if (rc < 0) {
            if (errno == EINTR)
                continue;

            throw Tsq::ErrnoException("write", errno);
        }
        len -= rc;
        buf += rc;
    }
}
