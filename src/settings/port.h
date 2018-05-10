// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/enums.h"

#include <QVector>

//
// Individual port forwarding item
//
struct PortFwdRule
{
    Tsq::PortForwardTaskType ltype, rtype;
    std::string laddr, lport;
    std::string raddr, rport;

    bool islocal;
    bool isauto;

private:
    QString m_spec, m_full;
    QString m_localStr, m_remoteStr;

public:
    void update();
    bool parseFull(const QString &full);
    bool parseSpec(QString spec);

    inline const QString& specStr() const { return m_spec; }
    inline const QString& fullStr() const { return m_full; }
    inline const QString& localStr() const { return m_localStr; }
    inline const QString& remoteStr() const { return m_remoteStr; }

    bool operator==(const PortFwdRule &other) const;
    inline bool operator!=(const PortFwdRule &other) const
    { return !this->operator==(other); }
};

//
// List of port forwarding items
//
class PortFwdList final: public QVector<PortFwdRule>
{
public:
    PortFwdList();
    PortFwdList(const QStringList &list);

    void parse(const QStringList &list);
    QStringList toStringList() const;
};
