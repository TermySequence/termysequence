// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "serverwatch.h"
#include "serverproxy.h"
#include "listener.h"
#include "reader.h"
#include "writer.h"
#include "conn.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "config.h"

/*
 * Local server
 */
ListenerWatch::ListenerWatch(TermReader *reader) :
    BaseWatch(g_listener, reader, WatchListener,
              ListenerWatchReleased, 0)
{
}

const Tsq::Uuid&
ListenerWatch::parentId() const
{
    return g_listener->id();
}

void
ListenerWatch::pushAnnounce()
{
    Tsq::ProtocolMarshaler m(TSQ_HANDSHAKE_COMPLETE);
    std::string response(std::move(m.result()));

    m.begin(TSQ_ANNOUNCE_SERVER);
    m.addUuidPair(g_listener->id(), m_reader->remoteId());
    m.addNumber(SERVER_VERSION);
    m.addNumberPair(0, g_listener->nTerms());
    m.addBytes(g_listener->commandGetAttributes());
    response.append(m.result());

    m_writer->submitResponse(std::move(response));
}

/*
 * Proxy
 */
ServerWatch::ServerWatch(ServerProxy *proxy, TermReader *reader) :
    BaseWatch(proxy->parent(), reader, WatchServer,
              TermWatchReleased, proxy->nHops()),
    m_proxy(proxy)
{
}

const Tsq::Uuid&
ServerWatch::parentId() const
{
    return m_proxy->id();
}

void
ServerWatch::pushAnnounce()
{
    Tsq::ProtocolMarshaler m(TSQ_ANNOUNCE_SERVER);
    m.addUuidPair(m_proxy->id(), m_proxy->hopId());
    m.addNumber(m_proxy->version());
    m.addNumberPair(hops, m_proxy->nTerms());

    {
        // two locks held
        ServerProxy::StateLock slock(m_proxy, false);
        m.addBytes(m_proxy->lockedGetAttributes());
    }

    // two locks held
    m_writer->submitResponse(std::move(m.result()));
}

void
ServerWatch::teardown()
{
    m_proxy->removeWatch(this);
}
