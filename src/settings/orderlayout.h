// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/wire.h"

#include <QString>
#include <QHash>

//
// Parsed state only
//
class OrderLayout
{
protected:
    bool m_ok;

    QHash<Tsq::Uuid,int> m_order;
    QHash<Tsq::Uuid,bool> m_hidden;

public:
    inline int order(const Tsq::Uuid &id) const
    { return m_order.value(id, -1); }
    inline bool hidden(const Tsq::Uuid &id) const
    { return m_hidden.value(id, false); }
    inline bool contains(const Tsq::Uuid &id) const
    { return m_order.contains(id); }
};

//
// Parser
//
class OrderLayoutReader final: public OrderLayout
{
private:
    QByteArray m_bytes;
    Tsq::ProtocolUnmarshaler m_unm;

    unsigned m_idx;

    bool parseItem();

public:
    OrderLayoutReader();
    void parse(const QByteArray &bytes);
};

//
// Serializer
//
class OrderLayoutWriter
{
private:
    Tsq::ProtocolMarshaler m_marshaler;

public:
    OrderLayoutWriter();

    void addServer(const Tsq::Uuid &id);
    void addTerm(const Tsq::Uuid &id, bool hidden);

    QByteArray result() const;
};
