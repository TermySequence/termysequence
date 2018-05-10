// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "connwatch.h"
#include "conn.h"
#include "writer.h"
#include "listener.h"
#include "lib/protocol.h"
#include "lib/wire.h"

ConnWatch::ConnWatch(ConnInstance *parent, TermReader *reader, WatchType type) :
    BaseWatch(parent, reader, type, TermWatchReleased, 0)
{
}

ConnWatch::ConnWatch(ConnInstance *parent, TermReader *reader) :
    BaseWatch(parent, reader, WatchConn, TermWatchReleased, 0)
{
}

const Tsq::Uuid&
ConnWatch::parentId() const
{
    auto *conn = static_cast<ConnInstance*>(m_parent);
    return conn->id();
}

void
ConnWatch::pushAnnounce()
{
    Tsq::ProtocolMarshaler m(TSQ_ANNOUNCE_CONN);
    m.addUuidPair(parent()->id(), g_listener->id());
    m.addNumber(0);
    // two locks held
    m.addBytes(parent()->commandGetAttributes());
    // two locks held
    m_writer->submitResponse(std::move(m.result()));
}
