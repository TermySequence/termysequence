// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "attributebase.h"

#include <list>
#include <unordered_set>
#include <unordered_map>
#include <atomic>

class ConnInstance;
class RawInstance;
class TermReader;
class TermWriter;
class TermMonitor;
class ServerProxy;
class TermProxy;
class TaskBase;

struct OwnershipChange {
    Tsq::Uuid oldId;
    Tsq::Uuid newId;
    AttributeMap attributes;

    inline OwnershipChange(const Tsq::Uuid &nId) :
        newId(nId) {}
    inline OwnershipChange(const Tsq::Uuid &oId, const Tsq::Uuid &nId) :
        oldId(oId), newId(nId) {}
};

class TermListener final: public AttributeBase
{
public:
    struct ClientInfo {
        TermWriter *writer;
        TermReader *reader;
        std::string announce;
        AttributeMap attributes;
        unsigned hops;
        unsigned flags;
    };

private:
    int m_initialrd, m_initialwd;
    unsigned m_ownerclients = 0;
    bool m_standalone;

    std::list<ConnInstance*> m_terms;
    std::list<TermReader*> m_readers;

    std::atomic_uint m_nTerms{0};
    std::atomic_uint m_nReaders{0};

    std::unordered_map<Tsq::Uuid,ConnInstance*> m_proxyConns;
    std::unordered_map<Tsq::Uuid,TermProxy*> m_proxyMap;
    std::unordered_map<Tsq::Uuid,ConnInstance*> m_serverConns;
    std::unordered_map<Tsq::Uuid,ServerProxy*> m_serverMap;

    std::unordered_map<Tsq::Uuid,const ClientInfo> m_clientMap;
    std::list<Tsq::Uuid> m_clientOrder;

    std::unordered_map<Tsq::Uuid,TaskBase*> m_taskMap;
    std::unordered_set<std::string> m_taskTargets;

private:
    BaseWatch* addConnWatch(ConnInstance *conn, TermReader *reader);
    BaseWatch* addServerWatch(ServerProxy *proxy, TermReader *reader);
    BaseWatch* addProxyWatch(TermProxy *proxy, TermReader *reader);
    void addReader(int rfd, int wfd);

    void threadMain();
    bool handleMultiFd(pollfd &pfd);
    bool handleInterrupt();

    bool handleWork(const WorkItem &item);
    void handleAddTerm(ConnInstance *conn);
    void handleAddConn(RawInstance *conn);
    bool handleRemoveTerm(ConnInstance *conn);
    void handleConfirmTerm(ConnInstance *conn);
    void handleConfirmReader(TermReader *reader);
    bool handleRemoveReader(TermReader *reader);
    void handleAddServer(ServerProxy *proxy);
    void handleRemoveServer(ServerProxy *proxy, int reason);
    void handleAddProxy(TermProxy *proxy);
    void handleRemoveProxy(TermProxy *proxy, int reason);
    void handleWatchReleased(BaseWatch *watch);
    void handleClearOwnership(Tsq::Uuid *id);
    void handleChangeOwnership(OwnershipChange *params);
    void handleAddTask(TaskBase *task);
    bool handleRemoveTask(TaskBase *task);

    bool checkCloseConditions();

public:
    TermListener(int initialrd, int initialwd, unsigned flavor);

    inline unsigned nTerms() const { return m_nTerms; }
    inline unsigned nReaders() const { return m_nReaders; }

    bool checkServer(const Tsq::Uuid &id);
    bool registerServer(const Tsq::Uuid &id, ServerProxy *proxy);
    void unregisterServer(const Tsq::Uuid &id, ServerProxy *proxy, int reason);
    int forwardToServer(const Tsq::Uuid &id, std::string &buf, Tsq::Uuid &hop);
    void forwardToServers(std::string &buf);

    bool registerProxy(const Tsq::Uuid &id, TermProxy *proxy);
    void unregisterProxy(const Tsq::Uuid &id, TermProxy *proxy, int reason);
    int forwardToTerm(const Tsq::Uuid &id, std::string &buf, Tsq::Uuid &hop);

    bool knownClient(const Tsq::Uuid &id);
    bool registerClient(const Tsq::Uuid &id, ClientInfo &info);
    void unregisterClient(const Tsq::Uuid &id, std::string &buf);
    int forwardToClient(const Tsq::Uuid &id, std::string &buf);
    void forwardToClients(std::string &buf);

    void inputTask(const Tsq::Uuid &taskId, std::string &buf);
    void answerTask(const Tsq::Uuid &taskId, int answer);
    void cancelTask(const Tsq::Uuid &taskId);
    void throttleTask(const Tsq::Uuid &taskId, const Tsq::Uuid &hopId);
    void resumeTasks(const Tsq::Uuid &hopId);

    void getOwnerAttributes(const Tsq::Uuid &id, AttributeMap &map) const;
    void getSenderAttributes(const Tsq::Uuid &id, AttributeMap &map) const;
    bool getClientAttribute(const Tsq::Uuid &id, std::string &inout) const;
};

enum ListenerWork {
    ListenerAddTerm,
    ListenerAddConn,
    ListenerConfirmTerm,
    ListenerRemoveTerm,
    ListenerAddReader,
    ListenerConfirmReader,
    ListenerRemoveReader,
    ListenerAddServer,
    ListenerRemoveServer,
    ListenerAddProxy,
    ListenerRemoveProxy,
    ListenerWatchReleased,
    ListenerClearOwnership,
    ListenerChangeOwnership,
    ListenerAddTask,
    ListenerRemoveTask,
};

extern TermListener *g_listener;

template<class T> static inline void
removeOne(std::list<T> &list, const T &value)
{
    for (auto i = list.begin(), j = list.end(); i != j; ++i)
        if (*i == value) {
            list.erase(i);
            break;
        }
}
