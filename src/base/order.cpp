// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "order.h"
#include "server.h"
#include "term.h"

#include <algorithm>

#define TR_TEXT1 TL("window-text", "%1 terminals (%2 hidden)")

TermOrder::TermOrder() :
    m_active(false),
    m_primary(false),
    m_populating(true)
{
}

struct TermOrderSorter
{
    const TermOrder &m_parent;
    bool operator()(const IdBase *a, const IdBase *b);
    TermOrderSorter(const TermOrder &parent): m_parent(parent) {}
};

bool
TermOrderSorter::operator()(const IdBase *a, const IdBase *b)
{
    int ao = m_parent.m_layout.order(a->id());
    int bo = m_parent.m_layout.order(b->id());

    if (ao != -1 && bo != -1)
        return (ao < bo);
    else if (ao != -1 && bo == -1)
        return true;
    else
        return false;
}

void
TermOrder::addServer(ServerInstance *server)
{
    m_termCounts.insert(server, std::pair<size_t,size_t>());
    m_order.append(server);
    m_servers.append(server);
    m_serverMap[server->id()] = server;

    if (m_populating && m_layout.contains(server->id()))
        sortServers();

    emit serverAdded(server);
}

void
TermOrder::removeServer(ServerInstance *server)
{
    m_serverMap.remove(server->id());
    m_termCounts.remove(server);
    m_order.removeOne(server);
    if (m_servers.removeOne(server))
        emit serverRemoved(server);
}

void
TermOrder::findTerm(TermInstance *term, int &tIdx, int &gIdx)
{
    gIdx = tIdx = m_terms.indexOf(term);

    for (auto server: qAsConst(m_servers)) {
        ++gIdx;
        if (server == term->server())
            break;
    }
}

void
TermOrder::findServer(ServerInstance *server, int &sIdx, int &tIdx, int &gIdx)
{
    sIdx = tIdx = gIdx = 0;

    for (auto i: qAsConst(m_servers)) {
        if (i == server)
            break;

        ++sIdx;
        auto total = m_termCounts[i].first;
        tIdx += total;
        gIdx += total + 1;
    }
}

inline void
TermOrder::updateServerHidden(ServerInstance *server)
{
    const auto &elt = m_termCounts[server];
    emit serverHidden(server, elt.first, elt.second);

    QString value = TR_TEXT1.arg(elt.first).arg(elt.second);
    server->setAttribute(g_attr_TSQT_COUNT, value);
}

void
TermOrder::addTerm(TermInstance *term)
{
    int terms = 0, servers = 0;
    ServerInstance *server = term->server();

    for (auto i: qAsConst(m_servers))
    {
        ++servers;
        terms += m_termCounts[i].first;
        if (i == server)
            break;
    }

    m_terms.insert(terms, term);
    m_order.insert(terms + servers, term);

    ++m_termCounts[server].first;
    m_termMap[term->id()] = term;

    if (m_layout.hidden(term->id())) {
        m_termHidden.insert(term);
        ++m_termCounts[server].second;
    }
    if (server->populating() && m_layout.contains(term->id())) {
        sortTerms(server);
    }

    emit termAdded(term);
    updateServerHidden(server);
}

void
TermOrder::removeTerm(TermInstance *term)
{
    int idx = m_terms.indexOf(term);
    ServerInstance *server = term->server();
    TermInstance *replacement = idx ? prevTerm(term) : nextTerm(term);

    if (replacement == term)
        replacement = nullptr;

    m_termMap.remove(term->id());

    --m_termCounts[server].first;
    if (m_termHidden.remove(term))
        --m_termCounts[server].second;

    m_terms.removeAt(idx);
    m_order.removeOne(term);

    emit termRemoved(term, replacement);
    updateServerHidden(server);
}

TermInstance *
TermOrder::sortTermsByProfile(ServerInstance *server, const QStringList &order)
{
    int count, tIdx, gIdx, pos;

    findServer(server, count, tIdx, gIdx);
    count = m_termCounts[server].first;
    ++gIdx;

    if (order.size() > count)
        return nullptr;

    pos = tIdx;
    for (auto &i: order) {
        if (m_terms[pos]->profileName() != i)
            for (int k = pos + 1; k < tIdx + count; ++k)
                if (m_terms[k]->profileName() == i) {
                    m_terms.move(k, pos);
                    break;
                }
    }

    pos = gIdx;
    for (auto &i: order) {
        if (static_cast<TermInstance*>(m_order[pos])->profileName() != i)
            for (int k = pos + 1; k < gIdx + count; ++k)
                if (static_cast<TermInstance*>(m_order[k])->profileName() == i) {
                    m_order.move(k, pos);
                    break;
                }
    }

    return m_terms[tIdx];
}

void
TermOrder::sortTerms(ServerInstance *server)
{
    int count, tIdx, gIdx;
    TermOrderSorter sorter(*this);

    findServer(server, count, tIdx, gIdx);
    count = m_termCounts[server].first;

    auto i = m_terms.begin() + tIdx;
    std::stable_sort(i, i + count, sorter);
    auto j = m_order.begin() + gIdx + 1;
    std::stable_sort(j, j + count, sorter);
}

void
TermOrder::sortServers()
{
    TermOrderSorter sorter(*this);

    if (m_servers.size() > 1)
    {
        QList<ServerInstance*> tmp = m_servers;
        std::stable_sort(tmp.begin(), tmp.end(), sorter);

        if (m_servers != tmp) {
            int tPos = 0, tEnd = m_terms.size() - 1;
            int gPos = 0, gEnd = m_order.size() - 1;
            int sEnd = m_servers.size() - 1;

            for (int i = 0; i <= sEnd; ++i)
            {
                while (m_servers[i] != tmp[i]) {
                    int n = m_termCounts[m_servers[i]].first;

                    for (int j = 0; j < n; ++j)
                        m_terms.move(tPos, tEnd);

                    for (int j = 0; j < n + 1; ++j)
                        m_order.move(gPos, gEnd);

                    m_servers.move(i, sEnd);
                }

                int n = m_termCounts[tmp[i]].first;
                tPos += n;
                gPos += n + 1;
            }
        }
    }
}

void
TermOrder::sortAll()
{
    TermOrderSorter sorter(*this);

    // Sort terminals
    auto i = m_terms.begin();
    auto j = m_order.begin();

    for (auto server: qAsConst(m_servers)) {
        int count = m_termCounts[server].first;

        ++j;
        std::stable_sort(i, i + count, sorter);
        std::stable_sort(j, j + count, sorter);
        i += count;
        j += count;
    }

    // Sort servers
    sortServers();
}

TermInstance *
TermOrder::firstTerm() const
{
    for (auto term: qAsConst(m_terms))
        if (!isHidden(term))
            return term;

    return nullptr;
}

TermInstance *
TermOrder::nextTerm(TermInstance *term) const
{
    int idx = m_terms.indexOf(term);
    int n = m_terms.size();

    for (int i = 1; i <= n; ++i) {
        TermInstance *tmp = m_terms[(idx + i) % n];
        if (!isHidden(tmp))
            return tmp;
    }

    return nullptr;
}

TermInstance *
TermOrder::nextTermWithin(TermInstance *term, ServerInstance *server) const
{
    int idx = m_terms.indexOf(term);
    int n = m_terms.size();

    for (int i = 1; i <= n; ++i) {
        TermInstance *tmp = m_terms[(idx + i) % n];
        if (!isHidden(tmp) && tmp->server() == server)
            return tmp;
    }

    return nullptr;
}

TermInstance *
TermOrder::prevTerm(TermInstance *term) const
{
    int idx = m_terms.indexOf(term);
    int n = m_terms.size();

    if (idx == -1)
        idx = 0;

    for (int i = n - 1; i >= 0; --i) {
        TermInstance *tmp = m_terms[(idx + i) % n];
        if (!isHidden(tmp))
            return tmp;
    }

    return nullptr;
}

ServerInstance *
TermOrder::nextServer(ServerInstance *server) const
{
    int idx = m_servers.indexOf(server);
    int n = m_servers.size();
    return n ? m_servers[(idx + 1) % n] : nullptr;
}

ServerInstance *
TermOrder::prevServer(ServerInstance *server) const
{
    int idx = m_servers.indexOf(server);
    int n = m_servers.size();
    return n ? m_servers[(idx + n - 1) % n] : nullptr;
}

void
TermOrder::setHidden(ServerInstance *server, bool hidden)
{
    int sIdx, tIdx, gIdx;
    findServer(server, sIdx, tIdx, gIdx);

    for (int i = 0; i < m_termCounts[server].first; ++i)
        setHidden(m_terms[tIdx + i], hidden);
}

void
TermOrder::setHidden(TermInstance *term, bool hidden)
{
    ServerInstance *server = term->server();

    if (hidden && !m_termHidden.contains(term)) {
        m_termHidden.insert(term);
        ++m_termCounts[server].second;
        updateServerHidden(server);
    }
    else if (!hidden && m_termHidden.remove(term)) {
        --m_termCounts[server].second;
        updateServerHidden(server);
    }
}

void
TermOrder::reorderTermForward(TermInstance *term)
{
    int tIdx, gIdx;
    bool changed = false;
    findTerm(term, tIdx, gIdx);

    while (tIdx > 0 && m_terms[tIdx - 1]->server() == term->server()) {
        changed = true;
        m_terms.swap(tIdx - 1, tIdx);
        m_order.swap(gIdx - 1, gIdx);

        if (isHidden(term) || !isHidden(m_terms[tIdx]))
            break;

        --tIdx;
        --gIdx;
    }

    if (changed)
        emit termReordered();
}

void
TermOrder::reorderTermBackward(TermInstance *term)
{
    int tIdx, gIdx;
    bool changed = false;
    findTerm(term, tIdx, gIdx);

    while (tIdx < m_terms.size() - 1 && m_terms[tIdx + 1]->server() == term->server()) {
        changed = true;
        m_terms.swap(tIdx + 1, tIdx);
        m_order.swap(gIdx + 1, gIdx);

        if (isHidden(term) || !isHidden(m_terms[tIdx]))
            break;

        ++tIdx;
        ++gIdx;
    }

    if (changed)
        emit termReordered();
}

void
TermOrder::reorderTermFirst(TermInstance *term)
{
    int tIdx, gIdx;
    bool changed = false;
    findTerm(term, tIdx, gIdx);

    while (tIdx > 0 && m_terms[tIdx - 1]->server() == term->server()) {
        changed = true;
        m_terms.swap(tIdx - 1, tIdx);
        m_order.swap(gIdx - 1, gIdx);
        --tIdx;
        --gIdx;
    }

    if (changed)
        emit termReordered();
}

void
TermOrder::reorderTermLast(TermInstance *term)
{
    int tIdx, gIdx;
    bool changed = false;
    findTerm(term, tIdx, gIdx);

    while (tIdx < m_terms.size() - 1 && m_terms[tIdx + 1]->server() == term->server()) {
        changed = true;
        m_terms.swap(tIdx + 1, tIdx);
        m_order.swap(gIdx + 1, gIdx);
        ++tIdx;
        ++gIdx;
    }

    if (changed)
        emit termReordered();
}

void
TermOrder::reorderServerForward(ServerInstance *server)
{
    int sIdx, tIdx, gIdx;
    findServer(server, sIdx, tIdx, gIdx);

    if (sIdx > 0) {
        ServerInstance *prev = m_servers[sIdx - 1];
        int n = m_termCounts[prev].first;
        int s = tIdx - n;
        int d = tIdx + m_termCounts[server].first - 1;

        for (int i = 0; i < n; ++i)
            m_terms.move(s, d);

        ++n;
        s = gIdx - n;
        d = gIdx + m_termCounts[server].first;

        for (int i = 0; i < n; ++i)
            m_order.move(s, d);

        m_servers.swap(sIdx - 1, sIdx);
        emit termReordered();
    }
}

void
TermOrder::reorderServerBackward(ServerInstance *server)
{
    int sIdx, tIdx, gIdx;
    findServer(server, sIdx, tIdx, gIdx);

    if (sIdx < m_servers.size() - 1) {
        ServerInstance *next = m_servers[sIdx + 1];
        int n = m_termCounts[server].first;
        int d = tIdx + n + m_termCounts[next].first - 1;

        for (int i = 0; i < n; ++i)
            m_terms.move(tIdx, d);

        ++n;
        d = gIdx + n + m_termCounts[next].first;

        for (int i = 0; i < n; ++i)
            m_order.move(gIdx, d);

        m_servers.swap(sIdx + 1, sIdx);
        emit termReordered();
    }
}

void
TermOrder::reorderServerFirst(ServerInstance *server)
{
    int sIdx, tIdx, gIdx;
    findServer(server, sIdx, tIdx, gIdx);

    if (sIdx > 0) {
        int n = m_termCounts[server].first;

        for (int i = 0; i < n; ++i)
            m_terms.move(tIdx + i, i);

        ++n;

        for (int i = 0; i < n; ++i)
            m_order.move(gIdx + i, i);

        m_servers.move(sIdx, 0);
        emit termReordered();
    }
}

void
TermOrder::reorderServerLast(ServerInstance *server)
{
    int sIdx, tIdx, gIdx;
    findServer(server, sIdx, tIdx, gIdx);

    if (sIdx < m_servers.size() - 1) {
        int n = m_termCounts[server].first;
        int d = m_terms.size() - 1;

        for (int i = 0; i < n; ++i)
            m_terms.move(tIdx, d);

        ++n;
        d = m_order.size() - 1;

        for (int i = 0; i < n; ++i)
            m_order.move(gIdx, d);

        m_servers.move(sIdx, m_servers.size() - 1);
        emit termReordered();
    }
}

void
TermOrder::findServerTargets(int oIdx, int dIdx, bool after, QObject *result[2])
{
    if (after)
        ++dIdx;
    if (dIdx <= oIdx) {
        while (qobject_cast<ServerInstance*>(m_order[dIdx]) == nullptr)
            --dIdx;

        result[0] = nullptr;
        result[1] = m_order[dIdx];

        while (--dIdx >= 0)
            if (!m_termHidden.contains(m_order[dIdx])) {
                result[0] = m_order[dIdx];
                break;
            }
    }
    else {
        --dIdx;
        while (dIdx < m_order.size() && qobject_cast<ServerInstance*>(m_order[dIdx]) == nullptr)
            ++dIdx;
        if (dIdx == m_order.size())
            while (qobject_cast<ServerInstance*>(m_order[--dIdx]) == nullptr);

        result[0] = m_order[dIdx];
        result[1] = nullptr;

        while (++dIdx < m_order.size())
            if (!m_termHidden.contains(m_order[dIdx])) {
                result[1] = m_order[dIdx];
                break;
            }
    }
}

void
TermOrder::moveServer(int oIdx, int dIdx, bool after)
{
    ServerInstance *o = static_cast<ServerInstance*>(m_order[oIdx]);
    ServerInstance *d;

    if (after)
        ++dIdx;
    if (dIdx <= oIdx) {
        while ((d = qobject_cast<ServerInstance*>(m_order[dIdx])) == nullptr)
            --dIdx;

        if (dIdx < oIdx) {
            int sIdx1, tIdx1, sIdx2, tIdx2, gIdx;

            findServer(o, sIdx1, tIdx1, gIdx);
            findServer(d, sIdx2, tIdx2, gIdx);

            int n = m_termCounts[o].first;

            for (int i = 0; i < n; ++i)
                m_terms.move(tIdx1 + i, tIdx2 + i);

            ++n;

            for (int i = 0; i < n; ++i)
                m_order.move(oIdx + i, dIdx + i);

            m_servers.move(sIdx1, sIdx2);
            emit termReordered();
        }
    }
    else {
        --dIdx;
        while (dIdx < m_order.size() && (d = qobject_cast<ServerInstance*>(m_order[dIdx])) == nullptr)
            ++dIdx;
        if (dIdx == m_order.size())
            while ((d = qobject_cast<ServerInstance*>(m_order[--dIdx])) == nullptr);

        if (dIdx > oIdx) {
            int sIdx1, tIdx1, sIdx2, tIdx2, gIdx;

            findServer(o, sIdx1, tIdx1, gIdx);
            findServer(d, sIdx2, tIdx2, gIdx);

            int n = m_termCounts[o].first;
            int m = m_termCounts[d].first;

            for (int i = 0; i < n; ++i)
                m_terms.move(tIdx1, tIdx2 + m - 1);

            ++n;
            ++m;

            for (int i = 0; i < n; ++i)
                m_order.move(oIdx, dIdx + m - 1);

            m_servers.move(sIdx1, sIdx2);
            emit termReordered();
        }
    }
}

void
TermOrder::findTermTargets(int oIdx, int dIdx, bool after, QObject *result[2])
{
    TermInstance *term = static_cast<TermInstance*>(m_order[oIdx]);
    int sIdx, tIdx, start, i;
    QList<int> indexes;

    findServer(term->server(), sIdx, tIdx, start);

    indexes.append(start++);

    for (i = 0; i < m_termCounts[term->server()].first; ++i)
        if (!isHidden(m_terms[tIdx + i]))
            indexes.append(start + i);

    sIdx = (sIdx < m_servers.size() - 1) ? start + i : -1;

    if (dIdx < indexes.front()) {
        i = 0;
    }
    else if (dIdx > indexes.back()) {
        i = indexes.size() - 1;
    }
    else {
        i = indexes.indexOf(dIdx);
        if (i == -1)
            return;
        if (!after && i > 0)
            --i;
    }

    if (sIdx != -1)
        indexes.append(sIdx);

    result[0] = m_order[indexes[i]];
    result[1] = (i < indexes.size() - 1) ? m_order[indexes[i + 1]] : nullptr;
}

void
TermOrder::moveTerm(int oIdx, int dIdx, bool after)
{
    TermInstance *term = static_cast<TermInstance*>(m_order[oIdx]);
    int sIdx, tIdx, start, i;
    QList<int> indexes;

    findServer(term->server(), sIdx, tIdx, start);

    indexes.append(start++);

    for (i = 0; i < m_termCounts[term->server()].first; ++i) {
        if (!isHidden(m_terms[tIdx + i]))
            indexes.append(start + i);
        if (m_terms[tIdx + i] == term)
            sIdx = tIdx + i;
    }

    if (dIdx < indexes.front()) {
        i = 0;
    }
    else if (dIdx > indexes.back()) {
        i = indexes.size() - 1;
    }
    else {
        i = indexes.indexOf(dIdx);
        if (i == -1)
            return;
        if (!after && i > 0)
            --i;
    }

    i = (indexes[i] < oIdx) ? indexes[i] + 1 : indexes[i];

    if (oIdx != i) {
        m_terms.move(sIdx, sIdx + i - oIdx);
        m_order.move(oIdx, i);
        emit termReordered();
    }
}

void
TermOrder::syncTo(const TermOrder *other)
{
    QSet<ServerInstance*> servers;

    for (auto i: m_servers)
        if (m_termCounts[i] != other->m_termCounts[i])
            servers.insert(i);

    m_servers = other->m_servers;
    m_terms = other->m_terms;
    m_order = other->m_order;
    m_termHidden = other->m_termHidden;
    m_termCounts = other->m_termCounts;

    for (auto i: servers) {
        const auto &elt = m_termCounts[i];
        emit serverHidden(i, elt.first, elt.second);
    }
    emit termReordered();
}

void
TermOrder::findDragTargets(const QByteArray &objectId, const QString &targetId, bool after, QObject *result[2])
{
    Tsq::Uuid objId, tgtId;
    int oIdx = -1, dIdx = -1;
    result[0] = result[1] = nullptr;

    if (!objId.parse(objectId.data()))
        return;
    if (!tgtId.parse(targetId.toLatin1().data()))
        return;

    // Find the objects in the global list
    for (int i = 0; i < m_order.size() && (dIdx == -1 || oIdx == -1); ++i) {
        if (m_order[i]->id() == tgtId)
            dIdx = i;
        if (m_order[i]->id() == objId)
            oIdx = i;
    }
    if (oIdx == -1 || dIdx == -1)
        return;

    // Is the draggee a server or a term
    if (m_serverMap.contains(objId))
        findServerTargets(oIdx, dIdx, after, result);
    else if (m_termMap.contains(objId))
        findTermTargets(oIdx, dIdx, after, result);
}

void
TermOrder::moveObject(const QByteArray &objectId, const QString &targetId, bool after)
{
    Tsq::Uuid objId, tgtId;
    int oIdx = -1, dIdx = -1;

    if (!objId.parse(objectId.data()))
        return;
    if (!tgtId.parse(targetId.toLatin1().data()))
        return;

    // Find the objects in the global list
    for (int i = 0; i < m_order.size() && (dIdx == -1 || oIdx == -1); ++i) {
        if (m_order[i]->id() == tgtId)
            dIdx = i;
        if (m_order[i]->id() == objId)
            oIdx = i;
    }
    if (oIdx == -1 || dIdx == -1)
        return;

    // Is the draggee a server or a term
    if (m_serverMap.contains(objId))
        moveServer(oIdx, dIdx, after);
    else if (m_termMap.contains(objId))
        moveTerm(oIdx, dIdx, after);
}
