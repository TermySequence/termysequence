// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/machine.h"

namespace Tsq { class RawProtocol; }
class ArgParser;

class CliConnection final: public Tsq::ProtocolCallback
{
private:
    Tsq::RawProtocol *m_machine;
    const ArgParser *m_args;

    int m_fd;
    int m_iter = 0;
    unsigned m_response;

    void doForward(int ifd, int ofd, const char *rbuf, size_t rlen);

public:
    bool run();

public:
    CliConnection(int fd, const ArgParser *args);
    ~CliConnection();

    bool protocolCallback(uint32_t command, uint32_t length, const char *body);
    void writeFd(const char *buf, size_t len);
};
