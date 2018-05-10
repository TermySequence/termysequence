// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/machine.h"

class TermReader;
namespace Tsq { class ServerHandshake; }

class ServerMachine final: public Tsq::ProtocolMachine
{
protected:
    TermReader *m_parent;

private:
    Tsq::ServerHandshake *m_handshake;
    bool m_expectingFd;
    bool m_gotFd;
    bool m_expectingConn;
    int m_connType;

    bool receiveFd(int fd);

public:
    ServerMachine(TermReader *parent);
    ~ServerMachine();

    virtual bool start();

    virtual bool connRead(int fd);
    virtual bool connRead(const char *buf, size_t len);
};
