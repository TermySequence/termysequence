// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QObject>
#include <QHash>
#include <QSet>

#include "app/attrbase.h"
#include "lib/types.h"
#include "settings/orderlayout.h"

class IdBase;
class ServerInstance;
class TermInstance;

class TermOrder: public QObject
{
    Q_OBJECT

    friend struct TermOrderSorter;

protected:
    QList<ServerInstance*> m_servers;
    QList<TermInstance*> m_terms;
    QHash<Tsq::Uuid,ServerInstance*> m_serverMap;
    QHash<Tsq::Uuid,TermInstance*> m_termMap;

private:
    QList<IdBase*> m_order;
    QSet<IdBase*> m_termHidden;
    QHash<ServerInstance*,std::pair<size_t,size_t>> m_termCounts;

    void updateServerHidden(ServerInstance *server);

    void findServerTargets(int objectIndex, int targetIndex, bool after, QObject *result[2]);
    void findTermTargets(int objectIndex, int targetIndex, bool after, QObject *result[2]);
    void moveTerm(int objectIndex, int targetIndex, bool after);
    void moveServer(int objectIndex, int targetIndex, bool after);

protected:
    OrderLayout m_layout;
    bool m_active;
    bool m_primary;
    bool m_populating;

    inline bool isEmpty() const { return m_terms.isEmpty(); }

    TermInstance* firstTerm() const;
    TermInstance* nextTerm(TermInstance *term) const;
    TermInstance* nextTermWithin(TermInstance *term, ServerInstance *server) const;
    TermInstance* prevTerm(TermInstance *term) const;
    void sortTerms(ServerInstance *server);
    TermInstance* sortTermsByProfile(ServerInstance *server, const QStringList &order);

    ServerInstance* nextServer(ServerInstance *server) const;
    ServerInstance* prevServer(ServerInstance *server) const;
    void sortServers();
    void sortAll();

signals:
    void serverAdded(ServerInstance *server);
    void serverRemoved(ServerInstance *server);
    void serverHidden(ServerInstance *server, size_t total, size_t hidden);

    void termAdded(TermInstance *term);
    void termRemoved(TermInstance *term, TermInstance *replacement);
    void termReordered();

public:
    TermOrder();

    inline const auto& order() const { return m_order; }
    inline const auto& servers() const { return m_servers; }
    inline const auto& terms() const { return m_terms; }

    inline bool primary() const { return m_primary; }
    inline void setPrimary(bool primary) { m_primary = primary; }

    inline ServerInstance* lookupServer(const Tsq::Uuid &id) { return m_serverMap.value(id); }
    inline TermInstance* lookupTerm(const Tsq::Uuid &id) { return m_termMap.value(id); }
    inline bool isHidden(IdBase *term) const { return m_termHidden.contains(term); }
    inline size_t totalCount(ServerInstance *server) const { return m_termCounts[server].first; }
    inline size_t hiddenCount(ServerInstance *server) const { return m_termCounts[server].second; }

    void setHidden(ServerInstance *server, bool hidden);
    void setHidden(TermInstance *term, bool hidden);

    void reorderServerForward(ServerInstance *server);
    void reorderServerBackward(ServerInstance *server);
    void reorderServerFirst(ServerInstance *server);
    void reorderServerLast(ServerInstance *server);
    void reorderTermForward(TermInstance *term);
    void reorderTermBackward(TermInstance *term);
    void reorderTermFirst(TermInstance *term);
    void reorderTermLast(TermInstance *term);

    void syncTo(const TermOrder *other);

    void findTerm(TermInstance *term, int &tIdx, int &gIdx);
    void findServer(ServerInstance *server, int &sIdx, int &tIdx, int &gIdx);
    void findDragTargets(const QByteArray &objectId, const QString &targetId, bool after, QObject *result[2]);
    void moveObject(const QByteArray &objectId, const QString &targetId, bool after);

public slots:
    void addServer(ServerInstance *server);
    void removeServer(ServerInstance *server);

    void addTerm(TermInstance *term);
    void removeTerm(TermInstance *term);
};
