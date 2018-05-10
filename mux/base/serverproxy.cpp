// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "serverproxy.h"
#include "conn.h"
#include "parsemap.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "os/logging.h"

ServerProxy::ServerProxy(ConnInstance *parent, const char *body, uint32_t length) :
    BaseProxy(parent, TermServerReleased)
{
    Tsq::ProtocolUnmarshaler unm(body, length);
    m_id = unm.parseUuid();
    m_hopId = unm.parseUuid();
    m_version = unm.parseNumber();
    m_nHops = unm.parseNumber() + 1;
    m_nTerms = m_startingTerms = unm.parseNumber();

    parseStringMap(unm, m_attributes);
}

void
ServerProxy::addTerm()
{
    if (m_startingTerms == 0)
        ++m_nTerms;
    else
        --m_startingTerms;
}

void
ServerProxy::wireCommand(uint32_t command, uint32_t length, const char *body)
{
    switch (command) {
    case TSQ_GET_SERVER_ATTRIBUTE:
        wireAttribute(body, length);
        break;
    default:
        LOGNOT("Term %p: unrecognized command %x\n", m_parent, command);
        break;
    }
}
