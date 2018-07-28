// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "proxywatch.h"
#include "termproxy.h"
#include "writer.h"
#include "conn.h"
#include "lib/protocol.h"
#include "lib/wire.h"

/*
 * Conn proxy
 */
ConnProxyWatch::ConnProxyWatch(TermProxy *proxy, TermReader *reader, WatchType type) :
    BaseWatch(proxy->parent(), reader, type,
              TermWatchReleased, proxy->nHops()),
    m_proxy(proxy)
{
}

ConnProxyWatch::ConnProxyWatch(TermProxy *proxy, TermReader *reader) :
    BaseWatch(proxy->parent(), reader, WatchConnProxy,
              TermWatchReleased, proxy->nHops()),
    m_proxy(proxy)
{
}

const Tsq::Uuid&
ConnProxyWatch::parentId() const
{
    return m_proxy->id();
}

void
ConnProxyWatch::pushAnnounce()
{
    Tsq::ProtocolMarshaler m(TSQ_ANNOUNCE_CONN);
    m.addUuidPair(m_proxy->id(), m_proxy->hopId());
    m.addNumber(hops);

    {
        // two locks held
        TermProxy::StateLock slock(m_proxy, false);
        m.addBytes(m_proxy->lockedGetAttributes());
    }

    // two locks held
    m_writer->submitResponse(std::move(m.result()));
}

void
ConnProxyWatch::teardown()
{
    m_proxy->removeWatch(this);
}

/*
 * Term proxy
 */
TermProxyWatch::TermProxyWatch(TermProxy *proxy, TermReader *reader) :
    ConnProxyWatch(proxy, reader, WatchTermProxy),
    state(static_cast<TermProxy*>(proxy))
{
}

void
TermProxyWatch::pushAnnounce()
{
    Tsq::ProtocolMarshaler m(TSQ_ANNOUNCE_TERM);
    m.addUuidPair(m_proxy->id(), m_proxy->hopId());
    m.addNumber(hops);

    {
        // two locks held
        TermProxy::StateLock slock(m_proxy, false);

        m.addNumberPair(m_proxy->size.width(), m_proxy->size.height());
        m.addBytes(m_proxy->lockedGetAttributes());

        for (const auto &i: m_proxy->proxyRows[0])
            state.changedRows[0].insert(i.first);
        for (const auto &i: m_proxy->proxyRows[1])
            state.changedRows[1].insert(i.first);
        for (const auto &i: m_proxy->proxyRegions)
            state.changedRegions.insert(i.first);

        files = m_proxy->files;
    }

    // two locks held
    m_writer->submitResponse(std::move(m.result()));
}

void
TermProxyWatch::pushFileUpdates(const StringMap &map)
{
    if (map.count(g_mtstr)) {
        files = map;
    } else {
        for (const auto &i: map)
            files[i.first] = i.second;
    }

    // do not activate here
}
