// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/enums.h"
#include "lib/uuid.h"
#include "lib/handshake.h"

#include <poll.h>

class ConnectorBase
{
protected:
    enum ConnectorState {
        ReadingRemoteHandshake, WritingRemoteHandshake, ReadingLocalHandshake,
        WritingLocalHandshake, ReadingLocalByte1, WritingDescriptor,
        ReadingLocalByte2, WritingHello, ReadingLocalResponse,
        WritingAttributes, ReadingConnId
    };

protected:
    int m_sd = -1;
    int m_state = ReadingRemoteHandshake;
    int m_protocolType;
    unsigned m_readCount = 0;
    bool m_pty;

    Tsq::ClientHandshake m_handshake{Tsq::Uuid::mt.buf, false};
    Tsq::ServerHandshake m_response{Tsq::Uuid::mt.buf};
    std::vector<pollfd> *m_fdp;
    std::string m_outbuf;
    std::string m_hello;
    std::string m_attributes;
    char m_connId[16];
    unsigned m_connLen = 0;

    Tsq::ConnectTaskError m_errtype;
    int m_errnum;
    int *m_exitStatusPtr;

    void setError(Tsq::ConnectTaskError type, bool noerrno = false, int errnum = 0);
    bool setHandshakeReadError(int rc);

    bool processConnectorFd(pollfd &pfd, int sendFd);
    void setAttribute(const std::string &key, const std::string &val);

    int writeData(int fd, std::string &data);

private:
    virtual bool readRemoteHandshake(pollfd &pfd) = 0;
    virtual bool writeRemoteHandshake(pollfd &pfd) = 0;
    bool readLocalHandshake();
    bool writeLocalHandshake();
    bool readLocalByte(int nextState);
    bool writeDescriptor(int sendFd);
    bool writeHello();
    bool readLocalResponse();
    bool writeAttributes();
    bool readConnId();
};
