// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "basewatch.h"
#include "eventstate.h"

class TermProxy;

/*
 * Conn proxy
 */
class ConnProxyWatch: public BaseWatch
{
protected:
    ConnProxyWatch(TermProxy *proxy, TermReader *reader, WatchType type);

    TermProxy *m_proxy;

    void pushAnnounce();

public:
    ConnProxyWatch(TermProxy *proxy, TermReader *reader);

    const Tsq::Uuid& parentId() const;

    void teardown();
};

/*
 * Term proxy
 */
class TermProxyWatch final: public ConnProxyWatch
{
public:
    // locked
    ProxyEventState state;
    AttributeMap files;

protected:
    void pushAnnounce();

public:
    TermProxyWatch(TermProxy *proxy, TermReader *reader);

    // locked
    void pushFileUpdates(const AttributeMap &map);
};
