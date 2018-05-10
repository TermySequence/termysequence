// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/machine.h"

class RawInstance;
namespace Tsq { class ClientHandshake; }

class ClientMachine final: public Tsq::ProtocolMachine
{
protected:
    RawInstance *m_parent;

private:
    Tsq::ClientHandshake *m_handshake;
    std::string m_attributes;
    uint32_t m_length = 0;
    bool m_handshaking = true;
    bool m_needLength = true;

    bool handshakeRead(const char *buf, size_t len);
    bool finish();

public:
    ClientMachine(RawInstance *parent, const char *buf, size_t len);
    ~ClientMachine();

    virtual bool connRead(int fd);
    virtual bool connRead(const char *buf, size_t len);
};
