// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "orderlayout.h"
#include "lib/exception.h"

#define ORDER_VERSION     1
#define ORDER_SERVER      0x80000000
#define ORDER_TERM        0x40000000
#define ORDER_HIDDEN      0x1

//
// Parser
//
OrderLayoutReader::OrderLayoutReader()
{
    m_ok = false;
    m_idx = 0;
}

bool
OrderLayoutReader::parseItem()
{
    unsigned type = m_unm.parseNumber();

    if (type & ORDER_SERVER) {
        Tsq::Uuid id = m_unm.parseUuid();
        m_order[id] = m_idx++;
        return true;
    }
    else if (type & ORDER_TERM) {
        Tsq::Uuid id = m_unm.parseUuid();
        m_order[id] = m_idx++;

        if (type & ORDER_HIDDEN)
            m_hidden[id] = true;

        return true;
    }

    return false;
}

void
OrderLayoutReader::parse(const QByteArray &bytes)
{
    m_ok = false;
    m_bytes = bytes;
    m_unm.begin(m_bytes.data(), m_bytes.size());

    try {
        unsigned version = m_unm.parseOptionalNumber();
        unsigned length = m_unm.parseOptionalNumber();

        if (version != ORDER_VERSION || length != m_unm.remainingLength())
            return;

        m_ok = true;

        while (m_unm.remainingLength())
            if (!parseItem()) {
                m_ok = false;
                break;
            }
    }
    catch (const Tsq::ProtocolException &e) {
        // bad parse
        m_ok = false;
    }

    if (!m_ok) {
        m_order.clear();
        m_hidden.clear();
    }
}

//
// Serializer
//
OrderLayoutWriter::OrderLayoutWriter()
{
    m_marshaler.begin(ORDER_VERSION);
}

void
OrderLayoutWriter::addServer(const Tsq::Uuid &id)
{
    m_marshaler.addNumber(ORDER_SERVER);
    m_marshaler.addUuid(id);
}

void
OrderLayoutWriter::addTerm(const Tsq::Uuid &id, bool hidden)
{
    m_marshaler.addNumber(ORDER_TERM | (hidden ? ORDER_HIDDEN : 0));
    m_marshaler.addUuid(id);
}

QByteArray
OrderLayoutWriter::result() const
{
    return QByteArray(m_marshaler.resultPtr(), m_marshaler.length());
}
