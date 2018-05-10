// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "basewatch.h"
#include "conn.h"

class ConnWatch: public BaseWatch
{
protected:
    ConnWatch(ConnInstance *parent, TermReader *reader, WatchType type);

    void pushAnnounce();

public:
    ConnWatch(ConnInstance *parent, TermReader *reader);

    inline ConnInstance* parent() { return static_cast<ConnInstance*>(m_parent); }

    const Tsq::Uuid& parentId() const;
};
