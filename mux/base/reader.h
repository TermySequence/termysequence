// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "threadbase.h"
#include "servermachine.h"
#include "basewatch.h"
#include "lib/stringmap.h"
#include "lib/uuid.h"

#include <set>
#include <unordered_set>
#include <unordered_map>

class TermWriter;
class ConnWatch;

class TermReader final: public ThreadBase, public Tsq::ProtocolCallback
{
    friend class ServerMachine;

private:
    // locked
    std::set<BaseWatch*,WatchSorter> m_watches;
    // unlocked
    std::unordered_map<Tsq::Uuid,BaseWatch*> m_terms;

private:
    TermWriter *m_writer;
    Tsq::ProtocolMachine *m_machine;

    int m_writeFd;
    int m_savedFd = -1;
    int m_savedWriteFd = -1;
    bool m_idleOut = false;
    bool m_cleanExit = false;

    Tsq::Uuid m_remoteId;
    std::unordered_set<Tsq::Uuid> m_knownClients, m_ignoredClients;
    SharedStringMap m_environ;

private:
    void threadMain();
    bool handleIdle();
    bool handleFd();
    void closefd();

    bool setMachine(Tsq::ProtocolMachine *newMachine, char protocolType);
    void setFd(int newrd, int newwd);
    void createConn(int protocolType, const char *buf, size_t len);
    void createConn(int protocolType, int newFd);

    bool handleWork(const WorkItem &item);
    void handleWatchAdded(BaseWatch *watch);
    void handleWatchRelease(BaseWatch *watch);

    void disconnect();

    void pushDisconnect(int reason);
    void pushThrottlePause(const char *body, ConnWatch *watch);

    bool handlePlainCommand(uint32_t command, uint32_t length, const char *body);
    void handleServerCommand(uint32_t command, uint32_t length, const char *body);
    void handleClientCommand(uint32_t command, uint32_t length, const char *body);
    void handleTermCommand(uint32_t command, uint32_t length, const char *body);

    void commandServerTimeRequest(const char *body);
    void commandServerGetAttributes(const char *body);
    void commandServerGetAttribute(const char *body, uint32_t length);
    void commandServerSetAttribute(const char *body, uint32_t length);
    void commandServerRemoveAttribute(const char *body, uint32_t length);
    void commandServerCreateTerm(const char *body, uint32_t length);
    void commandServerTaskPause(const char *body, uint32_t length);
    void commandServerTaskInput(const char *body, uint32_t length);
    void commandServerTaskAnswer(const char *body, uint32_t length);
    void commandServerCancelTask(const char *body, uint32_t length);
    void commandServerFileTask(uint32_t command, const char *body, uint32_t length);
    void commandServerMonitorInput(const char *body, uint32_t length);
    void commandServerClientAttribute(const char *body, uint32_t length);

    void commandClientAnnounce(const char *body, uint32_t length);
    void commandClientRemove(const char *body, uint32_t length);

    void commandTermGetAttributes(ConnWatch *watch, const char *body);
    void commandTermGetAttribute(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermSetAttribute(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermRemoveAttribute(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermInput(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermMouse(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermResizeBuffer(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermResizeTerm(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermDuplicate(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermReset(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermSendSignal(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermGetRows(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermGetImage(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermDownloadImage(ConnWatch *watch, const char *body, uint32_t length);
    void commandTermChangeOwner(ConnWatch *watch, const char *body);

    void commandRegionCreate(ConnWatch *watch, const char *body, uint32_t length);
    void commandRegionGet(ConnWatch *watch, const char *body, uint32_t length);
    void commandRegionRemove(ConnWatch *watch, const char *body, uint32_t length);

public:
    TermReader(int writeFd, StringMap &&environ);
    ~TermReader();

    inline TermWriter* writer() { return m_writer; }
    inline Tsq::ProtocolMachine* machine() { return m_machine; }
    inline const Tsq::Uuid& remoteId() const { return m_remoteId; }
    inline const auto& environ() const { return m_environ; }

    void setWatches(std::set<BaseWatch*,WatchSorter> &watches);
    void addWatch(BaseWatch *watch);
    void removeWatch(BaseWatch *watch);

    bool protocolCallback(uint32_t command, uint32_t length, const char *body);
    void writeFd(const char *buf, size_t len);

    static void pushTaskResume(const Tsq::Uuid &id);
};

enum ReaderWork {
    ReaderClose,
    ReaderWatchAdded,
    ReaderReleaseWatch,
    ReaderPostConfirm
};
