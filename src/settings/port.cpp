// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "port.h"

#include <QRegularExpression>

static const QRegularExpression s_startnum(L("\\A(\\d+):"));
static const QRegularExpression s_ybrackets(L("\\A\\[(.*?)\\](?::|\\z)"));
static const QRegularExpression s_nbrackets(L("\\A(.*?)(?::|\\z)"));

//
// Individual port forwarding item
//

void
PortFwdRule::update()
{
    QStringList slist, llist, rlist;
    QString tmp;

    slist.append(C("rlRL"[(isauto ? 2 : 0) | (islocal ? 1 : 0)]));

    slist.append(QString::number(ltype));
    llist.append(ltype == Tsq::PortForwardTCP ? A("TCP") : A("Unix"));
    tmp = laddr.find(':') != std::string::npos ? A("[%1]") : A("%1");
    tmp = tmp.arg(QString::fromStdString(laddr));
    slist.append(tmp);
    llist.append(tmp);

    if (ltype == Tsq::PortForwardTCP) {
        tmp = QString::fromStdString(lport);
        slist.append(tmp);
        llist.append(tmp);
    }

    slist.append(QString::number(rtype));
    rlist.append(rtype == Tsq::PortForwardTCP ? A("TCP") : A("Unix"));
    tmp = raddr.find(':') != std::string::npos ? A("[%1]") : A("%1");
    tmp = tmp.arg(QString::fromStdString(raddr));
    slist.append(tmp);
    rlist.append(tmp);

    if (rtype == Tsq::PortForwardTCP) {
        tmp = QString::fromStdString(rport);
        slist.append(tmp);
        rlist.append(tmp);
    }

    m_full = slist.join(':');
    slist.pop_front();
    m_spec = slist.join(':');
    m_localStr = llist.join(':');
    m_remoteStr = rlist.join(':');
}

bool
PortFwdRule::parseSpec(QString spec)
{
    // Unix format is ltype:laddr:rtype:raddr
    // TCP Format is ltype:laddr:lport:rtype:raddr:rport
    // Square brackets can surround TCP address field

    auto match = s_startnum.match(spec);
    if (!match.hasMatch())
        return false;
    ltype = (Tsq::PortForwardTaskType)match.captured(1).toInt();
    spec.remove(0, match.capturedLength());

    match = s_ybrackets.match(spec);
    if (match.hasMatch()) {
        laddr = match.captured(1).toStdString();
    } else {
        match = s_nbrackets.match(spec);
        if (!match.hasMatch())
            return false;
        laddr = match.captured(1).toStdString();
    }
    spec.remove(0, match.capturedLength());

    switch (ltype) {
    case Tsq::PortForwardTCP: {
        match = s_nbrackets.match(spec);
        if (!match.hasMatch())
            return false;
        lport = match.captured(1).toStdString();
        spec.remove(0, match.capturedLength());
        break;
    }
    case Tsq::PortForwardUNIX:
        break;
    default:
        return false;
    }

    match = s_startnum.match(spec);
    if (!match.hasMatch())
        return false;
    rtype = (Tsq::PortForwardTaskType)match.captured(1).toInt();
    spec.remove(0, match.capturedLength());

    match = s_ybrackets.match(spec);
    if (match.hasMatch()) {
        raddr = match.captured(1).toStdString();
    } else {
        match = s_nbrackets.match(spec);
        if (!match.hasMatch())
            return false;
        raddr = match.captured(1).toStdString();
    }
    spec.remove(0, match.capturedLength());

    switch (rtype) {
    case Tsq::PortForwardTCP: {
        match = s_nbrackets.match(spec);
        if (!match.hasMatch())
            return false;
        rport = match.captured(1).toStdString();
        break;
    }
    case Tsq::PortForwardUNIX:
        break;
    default:
        return false;
    }

    update();
    return true;
}

bool
PortFwdRule::parseFull(const QString &full)
{
    if (full.size() < 2 || full.at(1) != ':')
        return false;

    switch (full.at(0).unicode()) {
    case 'l':
        islocal = true;
        isauto = false;
        break;
    case 'r':
        isauto = islocal = false;
        break;
    case 'L':
        isauto = islocal = true;
        break;
    case 'R':
        islocal = false;
        isauto = true;
        break;
    default:
        return false;
    }

    return parseSpec(full.mid(2));
}

bool
PortFwdRule::operator==(const PortFwdRule &o) const
{
    return (ltype == o.ltype) && (rtype == o.rtype) &&
        (laddr == o.laddr) && (raddr == o.raddr) &&
        (lport == o.lport) && (rport == o.rport) &&
        (islocal == o.islocal);
}

//
// List of port forwarding items
//
void
PortFwdList::parse(const QStringList &list)
{
    for (auto &i: list) {
        PortFwdRule pfc;
        if (pfc.parseFull(i))
            append(pfc);
    }
}

PortFwdList::PortFwdList(const QStringList &list)
{
    parse(list);
}

PortFwdList::PortFwdList()
{
}

QStringList
PortFwdList::toStringList() const
{
    QStringList result;

    for (auto &i: *this) {
        result.append(i.fullStr());
    }

    return result;
}
