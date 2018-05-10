// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "attributebase.h"
#include "lib/machine.h"

#include <unordered_set>
#include <unordered_map>

class TermOutput;
class ServerProxy;
class TermProxy;
struct OwnershipChange;

enum DisconnectFlags {
    DisActive = 1,
    DisReport = 2,
    DisLocked = 4,
};

class ConnInstance: public AttributeBase, public Tsq::ProtocolCallback
{
protected:
    TermOutput *m_output;
    Tsq::ProtocolMachine *m_machine = nullptr;

    Tsq::Uuid m_remoteId;
    Tsq::Uuid m_owner;
    Tsq::Uuid m_sender;

    const bool m_isTerm;
    bool m_haveConnection = false;
    bool m_removingConnection = false;
    bool m_haveExitStatus = false;
    bool m_closing = false;
    bool m_reportDisconnect;

private:
    std::unordered_set<Tsq::Uuid> m_knownServers;
    std::unordered_set<Tsq::Uuid> m_ignoredServers;
    std::unordered_map<Tsq::Uuid,ServerProxy*> m_activeServers;
    std::unordered_set<ServerProxy*> m_removingServers;

    std::unordered_set<Tsq::Uuid> m_knownTerms;
    std::unordered_set<Tsq::Uuid> m_ignoredTerms;
    std::unordered_map<Tsq::Uuid,TermProxy*> m_activeTerms;
    std::unordered_set<TermProxy*> m_removingTerms;

private:
    void doSetOwner(const Tsq::Uuid &owner, AttributeMap &map);
    void doSetSender(const Tsq::Uuid &sender, AttributeMap &map);

    void wireDisconnect(const char *body, uint32_t length);
    void wireServerAnnounce(const char *body, uint32_t length);
    void wireServerRemove(const Tsq::Uuid &id, uint32_t length, const char *body);
    void wireTermAnnounce(uint32_t command, uint32_t length, const char *body);
    void wireTermRemove(const Tsq::Uuid &id, uint32_t length, const char *body);

    bool wirePlainCommand(uint32_t command, uint32_t length, const char *body);
    bool wireServerCommand(uint32_t command, uint32_t length, const char *body);
    bool wireClientCommand(uint32_t command, uint32_t length, const char *body);
    bool wireTermCommand(uint32_t command, uint32_t length, const char *body);

    virtual void postConnect() = 0;
    virtual void postDisconnect(bool locked) = 0;

    void pushKeepalive();
    void pushTaskPause(const char *body);
    bool checkCloseConditions(bool noWatches);

protected:
    void closefd();
    void disconnect(unsigned flags, int reason);
    bool handleClose(unsigned flags, int reason);
    bool handleWatchReleased(BaseWatch *watch);
    bool handleServerReleased(ServerProxy *proxy);
    void handleProxyReleased(TermProxy *proxy);

    void pushChannelTest();
    void pushConfigureKeepalive(unsigned timeout);

public:
    ConnInstance(const char *name, bool isTerm);
    ~ConnInstance();

    inline TermOutput* output() { return m_output; }
    inline Tsq::ProtocolMachine* machine() { return m_machine; }
    inline bool isTerm() const { return m_isTerm; }

    bool testOwner(const Tsq::Uuid &owner);
    bool testSender(const Tsq::Uuid &owner);
    void setOwner(const Tsq::Uuid &owner);
    bool changeOwner(OwnershipChange *params);
    void clearOwner(const Tsq::Uuid &owner);

    bool protocolCallback(uint32_t command, uint32_t length, const char *body);
    void writeFd(const char *buf, size_t len);

    bool addWatch(BaseWatch *watch);

    static void pushThrottleResume(const Tsq::Uuid &id);
};

enum TermWork {
    TermClose,
    TermDisconnect,
    TermWatchAdded,
    TermWatchReleased,
    TermServerReleased,
    TermProxyReleased,
    TermProcessExited, // ProcessExited common value
    TermResizeTerm,
    TermResizeBuffer,
    TermCreateRegion,
    TermRemoveRegion,
    TermReset,
    TermMoveMouse,
    TermInputSent,
    TermSetScrollLock,
    TermSignal,
};
