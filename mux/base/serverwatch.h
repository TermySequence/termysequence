// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "basewatch.h"

class ServerProxy;

/*
 * Local server
 */
class ListenerWatch final: public BaseWatch
{
protected:
    void pushAnnounce();

public:
    ListenerWatch(TermReader *reader);

    const Tsq::Uuid& parentId() const;
};

/*
 * Proxy
 */
class ServerWatch final: public BaseWatch
{
private:
    ServerProxy *m_proxy;

protected:
    void pushAnnounce();

public:
    ServerWatch(ServerProxy *proxy, TermReader *reader);

    const Tsq::Uuid& parentId() const;

    void teardown();
};
