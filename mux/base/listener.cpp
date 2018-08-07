// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "listener.h"
#include "reader.h"
#include "writer.h"
#include "term.h"
#include "output.h"
#include "termwatch.h"
#include "serverproxy.h"
#include "serverwatch.h"
#include "termproxy.h"
#include "proxywatch.h"
#include "raw.h"
#include "zombies.h"
#include "monitor.h"
#include "taskbase.h"
#include "exception.h"
#include "os/conn.h"
#include "os/time.h"
#include "os/attr.h"
#include "os/status.h"
#include "os/eventfd.h"
#include "os/logging.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/attrstr.h"
#include "config.h"

#include <cassert>
#include <unistd.h>

TermListener *g_listener;

TermListener::TermListener(int initialrd, int initialwd, unsigned flavor) :
    AttributeBase(nullptr),
    m_initialrd(initialrd),
    m_initialwd(initialwd),
    m_standalone(flavor == Tsq::FlavorStandalone)
{
    std::vector<int> pids;
    m_pid = getpid();

    int64_t before = osWalltime();
    m_environ = osGetProcessEnvironment(m_pid);
    osIdentity(m_id, pids);
    osAttributes(m_attributes, pids, true);
    int64_t after = osWalltime();

    m_attributes[Tsq::attr_MACHINE_ID] = m_id.str();
    m_attributes[Tsq::attr_PID] = std::to_string(m_pid);
    m_attributes[Tsq::attr_PRODUCT] = SERVER_NAME " " PROJECT_VERSION;
    m_attributes[Tsq::attr_FLAVOR] = std::to_string(flavor);
    m_attributes[Tsq::attr_STARTED] = std::to_string(after);

    if (after - before > ATTRIBUTE_SCRIPT_WARN)
        LOGWRN("Warning: slow identity or attribute scripts "
               "delayed startup by %" PRId64 "ms\n", after - before);

    int mix = getuid();
    m_id.combine((uint32_t)(m_standalone ? -mix - 1 : mix));
    m_attributes[Tsq::attr_ID] = m_id.str();

    // these are leaked
    g_reaper = new TermReaper(pids);
    g_monitor = new TermMonitor();

    osInitEvent(s_reloadFd);
}

/*
 * Other threads
 */
bool
TermListener::checkServer(const Tsq::Uuid &id)
{
    Lock lock(this);

    if (m_id == id || m_serverConns.count(id)) {
        LOGDBG("Listener: server %s already registered\n", id.str().c_str());
        return false;
    }

    return true;
}

bool
TermListener::registerServer(const Tsq::Uuid &id, ServerProxy *proxy)
{
    if (id == m_id)
        return false;

    FlexLock lock(this);

    if (m_serverConns.count(id)) {
        LOGDBG("Listener: server %s already registered\n", id.str().c_str());
        return false;
    }
    else {
        auto *conn = proxy->parent();
        m_serverConns.emplace(id, conn);

        for (const auto &i: m_clientMap) {
            std::string copy(i.second.announce);
            conn->output()->submitCommand(std::move(copy));
        }

        LOGDBG("Listener: server %s registered (%zd clients announced)\n", id.str().c_str(), m_clientMap.size());
        sendWorkAndUnlock(lock, ListenerAddServer, proxy);
        return true;
    }
}

void
TermListener::unregisterServer(const Tsq::Uuid &id, ServerProxy *proxy, int reason)
{
    FlexLock lock(this);

    auto i = m_serverConns.find(id);
    if (i != m_serverConns.end()) {
        m_serverConns.erase(i);
        LOGDBG("Listener: server %s unregistered\n", id.str().c_str());
        sendWorkAndUnlock(lock, ListenerRemoveServer, proxy, reason);
    }
}

int
TermListener::forwardToServer(const Tsq::Uuid &id, std::string &buf, Tsq::Uuid &hop)
{
    Lock lock(this);

    auto i = m_serverConns.find(id);
    if (i != m_serverConns.end()) {
        hop = i->second->id();
        // two locks held
        return i->second->output()->submitCommand(std::move(buf));
    } else {
        return -1;
    }
}

void
TermListener::forwardToServers(std::string &buf)
{
    std::unordered_set<ConnInstance*> sent;

    Lock lock(this);

    for (const auto &i: m_serverConns) {
        if (sent.count(i.second) == 0) {
            sent.insert(i.second);
            std::string copy(buf);
            // two locks held
            i.second->output()->submitCommand(std::move(copy));
        }
    }
}

bool
TermListener::registerProxy(const Tsq::Uuid &id, TermProxy *proxy)
{
    FlexLock lock(this);

    if (m_proxyConns.count(id)) {
        LOGDBG("Listener: proxy %s already registered\n", id.str().c_str());
        return false;
    } else {
        m_proxyConns.emplace(id, proxy->parent());
        LOGDBG("Listener: proxy %s registered\n", id.str().c_str());
        sendWorkAndUnlock(lock, ListenerAddProxy, proxy);
        return true;
    }
}

void
TermListener::unregisterProxy(const Tsq::Uuid &id, TermProxy *proxy, int reason)
{
    FlexLock lock(this);

    auto i = m_proxyConns.find(id);
    if (i != m_proxyConns.end()) {
        m_proxyConns.erase(i);
        LOGDBG("Listener: proxy %s unregistered\n", id.str().c_str());
        sendWorkAndUnlock(lock, ListenerRemoveProxy, proxy, reason);
    }
}

int
TermListener::forwardToTerm(const Tsq::Uuid &id, std::string &buf, Tsq::Uuid &hop)
{
    Lock lock(this);

    auto i = m_proxyConns.find(id);
    if (i != m_proxyConns.end()) {
        hop = i->second->id();
        // two locks held
        return i->second->output()->submitCommand(std::move(buf));
    } else {
        return -1;
    }
}

static inline void
copyOwnerAttributes(const Tsq::Uuid &id, const StringMap &src, StringMap &dst)
{
    for (const auto &i: src)
        if (i.first.front() != '_') {
            dst[Tsq::attr_OWNER_PREFIX + i.first] = i.second;
            dst[Tsq::attr_SENDER_PREFIX + i.first] = i.second;
        }

    dst[Tsq::attr_SENDER_ID] = dst[Tsq::attr_OWNER_ID] = id.str();
}

void
TermListener::getOwnerAttributes(const Tsq::Uuid &id, StringMap &dst) const
{
    Lock lock(this);

    auto i = m_clientMap.find(id);
    if (i != m_clientMap.end()) {
        copyOwnerAttributes(id, i->second.attributes, dst);
    } else {
        dst[Tsq::attr_SENDER_ID] = dst[Tsq::attr_OWNER_ID] = id.str();
    }
}

void
TermListener::getOwnerAttributes(const Tsq::Uuid &id, OwnershipInfo &oi) const
{
    Lock lock(this);

    auto i = m_clientMap.find(id);
    if (i != m_clientMap.end()) {
        copyOwnerAttributes(id, i->second.attributes, oi.attributes);
        oi.environ = i->second.reader->environ();
    } else {
        oi.attributes[Tsq::attr_SENDER_ID] = oi.attributes[Tsq::attr_OWNER_ID] = id.str();
    }
}

void
TermListener::getSenderAttributes(const Tsq::Uuid &id, StringMap &dst) const
{
    Lock lock(this);

    auto i = m_clientMap.find(id);
    if (i != m_clientMap.end())
        for (const auto &i: i->second.attributes)
            if (i.first.front() != '_')
                dst[Tsq::attr_SENDER_PREFIX + i.first] = i.second;

    dst[Tsq::attr_SENDER_ID] = id.str();
}

bool
TermListener::getClientAttribute(const Tsq::Uuid &id, std::string &inout) const
{
    std::string spec = inout;
    spec.push_back('\0');

    Lock lock(this);

    auto i = m_clientMap.find(id);
    if (i == m_clientMap.end())
        return false;

    auto j = i->second.attributes.find(inout);
    if (j != i->second.attributes.end()) {
        spec.append(j->second);
        spec.push_back('\0');
    }

    inout = std::move(spec);
    return true;
}

bool
TermListener::knownClient(const Tsq::Uuid &id)
{
    Lock lock(this);
    return m_clientMap.count(id);
}

bool
TermListener::registerClient(const Tsq::Uuid &id, ClientInfo &info)
{
    FlexLock lock(this);

    if (m_clientMap.count(id)) {
        LOGDBG("Listener: client %s already registered (retval 0)\n", id.str().c_str());
        return false;
    }

    for (auto i = m_clientOrder.begin(), j = m_clientOrder.end();; ++i) {
        if (i == j || m_clientMap[*i].hops > info.hops) {
            m_clientOrder.insert(i, id);
            break;
        }
    }

    const auto i = m_clientMap.emplace(id, std::move(info)).first;

    LOGDBG("Listener: client %s registered (%d hops)\n", id.str().c_str(), info.hops);

    if (info.flags & Tsq::TakeOwnership && m_ownerclients++ == 0) {
        OwnershipChange *tmp = new OwnershipChange(id);
        copyOwnerAttributes(id, i->second.attributes, tmp->attributes);
        tmp->environ = i->second.reader->environ();
        sendWorkAndUnlock(lock, ListenerChangeOwnership, tmp);
    }

    return true;
}

void
TermListener::unregisterClient(const Tsq::Uuid &id, std::string &buf)
{
    std::unordered_set<ConnInstance*> sent;
    ListenerWork work;
    void *payload = nullptr;

    FlexLock lock(this);
    auto i = m_clientMap.find(id);

    if (i != m_clientMap.end()) {
        if (i->second.flags & Tsq::TakeOwnership)
            --m_ownerclients;

        m_clientMap.erase(i);
        removeOne(m_clientOrder, id);

        if (m_ownerclients) {
            for (const auto &newId: m_clientOrder) {
                const auto &info = m_clientMap[newId];
                if (info.flags & Tsq::TakeOwnership) {
                    OwnershipChange *tmp = new OwnershipChange(id, newId);
                    copyOwnerAttributes(newId, info.attributes, tmp->attributes);
                    tmp->environ = info.reader->environ();

                    work = ListenerChangeOwnership;
                    payload = tmp;
                    break;
                }
            }
        } else {
            work = ListenerClearOwnership;
            payload = new Tsq::Uuid(id);
        }
    }

    for (const auto &i: m_serverConns) {
        if (sent.count(i.second) == 0) {
            sent.insert(i.second);
            std::string copy(buf);
            // two locks held
            i.second->output()->submitCommand(std::move(copy));
        }
    }
    for (const auto &i: m_taskMap) {
        if (i.second->clientId() == id)
            i.second->stop(TSQ_STATUS_LOST_CONN);
    }

    LOGDBG("Listener: client %s unregistered (%zd messages sent)\n", id.str().c_str(), sent.size());

    if (payload) {
        sendWorkAndUnlock(lock, work, payload);
    }
}

int
TermListener::forwardToClient(const Tsq::Uuid &id, std::string &buf)
{
    Lock lock(this);

    auto i = m_clientMap.find(id);
    if (i != m_clientMap.end()) {
        // two locks held
        return i->second.writer->submitResponse(std::move(buf));
    } else {
        return -1;
    }
}

void
TermListener::forwardToClients(std::string &buf)
{
    std::unordered_set<TermWriter*> sent;

    Lock lock(this);

    for (const auto &i: m_clientMap) {
        if (sent.count(i.second.writer) == 0) {
            sent.insert(i.second.writer);
            std::string copy(buf);
            // two locks held
            i.second.writer->submitResponse(std::move(copy));
        }
    }
}

void
TermListener::inputTask(const Tsq::Uuid &taskId, std::string &buf)
{
    Lock lock(this);

    auto i = m_taskMap.find(taskId);
    if (i != m_taskMap.end())
        i->second->sendInput(buf);
}

void
TermListener::answerTask(const Tsq::Uuid &taskId, int answer)
{
    Lock lock(this);

    auto i = m_taskMap.find(taskId);
    if (i != m_taskMap.end())
        i->second->sendWork(TaskAnswer, answer);
}

void
TermListener::cancelTask(const Tsq::Uuid &taskId)
{
    Lock lock(this);

    auto i = m_taskMap.find(taskId);
    if (i != m_taskMap.end())
        i->second->stop(TSQ_STATUS_NORMAL);
}

void
TermListener::throttleTask(const Tsq::Uuid &taskId, const Tsq::Uuid &hopId)
{
    Lock lock(this);

    auto i = m_taskMap.find(taskId);
    if (i != m_taskMap.end() && i->second->throttlable())
        i->second->pause(hopId);
}

void
TermListener::resumeTasks(const Tsq::Uuid &hopId)
{
    Lock lock(this);

    for (const auto &i: m_taskMap)
        if (i.second->throttlable())
            i.second->resume(hopId);
}

/*
 * This thread
 */
inline bool
TermListener::checkCloseConditions()
{
    // Conditions for close:
    // a) asked to close (i.e. interrupted) and no terminals, readers, or tasks
    // or
    // b) in standalone mode, and no readers remaining

    if (interrupted())
        return !m_terms.empty() || !m_readers.empty() || !m_taskMap.empty();
    else
        return !m_standalone || !m_readers.empty();
}

BaseWatch *
TermListener::addConnWatch(ConnInstance *conn, TermReader *reader)
{
    ConnWatch *watch = conn->isTerm() ?
        new TermWatch(static_cast<TermInstance*>(conn), reader) :
        new ConnWatch(conn, reader);

    BaseWatch::FlexLock wlock(watch);

    if (conn->addWatch(watch)) {
        return watch;
    } else {
        wlock.unlock();
        delete watch;
        return nullptr;
    }
}

BaseWatch *
TermListener::addServerWatch(ServerProxy *proxy, TermReader *reader)
{
    auto *watch = new ServerWatch(proxy, reader);
    BaseWatch::FlexLock wlock(watch);

    if (proxy->addWatch(watch)) {
        return watch;
    } else {
        wlock.unlock();
        delete watch;
        return nullptr;
    }
}

BaseWatch *
TermListener::addProxyWatch(TermProxy *proxy, TermReader *reader)
{
    ConnProxyWatch *watch = proxy->isTerm() ?
        new TermProxyWatch(proxy, reader) :
        new ConnProxyWatch(proxy, reader);

    BaseWatch::FlexLock wlock(watch);

    if (proxy->addWatch(watch)) {
        return watch;
    } else {
        wlock.unlock();
        delete watch;
        return nullptr;
    }
}

void
TermListener::addReader(int rfd, int wfd, StringMap environ)
{
    TermReader *reader = new TermReader(wfd, std::move(environ));
    m_readers.push_back(reader);
    ++m_nReaders;

    reader->start(rfd);
}

bool
TermListener::handleMultiFd(pollfd &pfd)
{
    if (pfd.fd == m_fd) {
        int connfd;

        try {
            connfd = osAccept(m_fd, nullptr, nullptr);
        }
        catch (const std::exception &e) {
            LOGERR("%s\n", e.what());
            return false;
        }

        if (connfd >= 0)
            try {
                int pid = osLocalCredsCheck(connfd);
                StringMap environ = osGetProcessEnvironment(pid);
                addReader(connfd, connfd, environ);
            }
            catch (const std::exception &e) {
                LOGERR("%s\n", e.what());
                close(connfd);
            }
    }
    else {
        osClearEvent(pfd.fd);
        g_monitor->sendWork(MonitorRestart, 0);
    }

    return true;
}

void
TermListener::handleAddTerm(ConnInstance *conn)
{
    m_terms.push_back(conn);
    ++m_nTerms;
    handleConfirmTerm(conn);
    conn->start(-1);
}

void
TermListener::handleAddConn(RawInstance *conn)
{
    m_terms.push_back(conn);
    ++m_nTerms;

    conn->start(conn->fd());
}

bool
TermListener::handleRemoveTerm(ConnInstance *conn)
{
    LOGDBG("Listener: removing terminal %p %s\n", conn, conn->id().str().c_str());

    --m_nTerms;
    removeOne(m_terms, conn);
    conn->join();

    if (conn->noWatches()) {
        delete conn;
    } else {
        // SERIOUS error here.
        conn->closeevents();
        LOGCRT("Listener: a terminal stopped with watches still active! It has been leaked!\n");
    }

    return checkCloseConditions();
}

void
TermListener::handleConfirmTerm(ConnInstance *conn)
{
    LOGDBG("Listener: adding terminal %p %s\n", conn, conn->id().str().c_str());
    conn->confirm();

    BaseWatch *watch;

    for (auto reader: m_readers)
        if (reader->confirmed() && (watch = addConnWatch(conn, reader)))
            reader->addWatch(watch);
}

void
TermListener::handleConfirmReader(TermReader *reader)
{
    LOGDBG("Listener: added a reader %p %s\n", reader, reader->remoteId().str().c_str());

    BaseWatch *watch = new ListenerWatch(reader);
    {
        Lock lock(this);
        m_watches.emplace(watch);
    }
    std::set<BaseWatch*,WatchSorter> set = { watch };

    for (auto term: m_terms)
        if (term->confirmed() && (watch = addConnWatch(term, reader)))
            set.emplace(watch);

    for (const auto &i: m_serverMap)
        if ((watch = addServerWatch(i.second, reader)))
            set.emplace(watch);

    for (const auto &i: m_proxyMap)
        if ((watch = addProxyWatch(i.second, reader)))
            set.emplace(watch);

    reader->setWatches(set);
    reader->confirm();
    reader->sendWork(ReaderPostConfirm, 0);
}

void
TermListener::handleAddServer(ServerProxy *proxy)
{
    LOGDBG("Listener: adding server %p %s\n", proxy, proxy->id().str().c_str());

    m_serverMap.emplace(proxy->id(), proxy);

    BaseWatch *watch;

    for (auto reader: m_readers)
        if (reader->confirmed() && (watch = addServerWatch(proxy, reader)))
            reader->addWatch(watch);
}

void
TermListener::handleRemoveServer(ServerProxy *proxy, int reason)
{
    LOGDBG("Listener: removing server %p %s\n", proxy, proxy->id().str().c_str());

    m_serverMap.erase(proxy->id());

    proxy->requestRelease(reason);
}

void
TermListener::handleAddProxy(TermProxy *proxy)
{
    LOGDBG("Listener: adding proxy %p %s\n", proxy, proxy->id().str().c_str());

    m_proxyMap.emplace(proxy->id(), proxy);

    assert(m_serverMap.count(proxy->hopId()));
    m_serverMap.at(proxy->hopId())->addTerm();

    BaseWatch *watch;

    for (auto reader: m_readers)
        if (reader->confirmed() && (watch = addProxyWatch(proxy, reader)))
            reader->addWatch(watch);
}

void
TermListener::handleRemoveProxy(TermProxy *proxy, int reason)
{
    LOGDBG("Listener: removing proxy %p %s\n", proxy, proxy->id().str().c_str());

    assert(m_serverMap.count(proxy->hopId()));
    m_serverMap.at(proxy->hopId())->removeTerm();

    m_proxyMap.erase(proxy->id());

    proxy->requestRelease(reason);
}

bool
TermListener::handleRemoveReader(TermReader *reader)
{
    LOGDBG("Listener: removing reader %p %s\n", reader, reader->remoteId().str().c_str());

    --m_nReaders;
    removeOne(m_readers, reader);
    reader->join();
    delete reader;

    return checkCloseConditions();
}

void
TermListener::handleWatchReleased(BaseWatch *watch)
{
    {
        Lock lock(this);
        m_watches.erase(watch);
    }

    delete watch;
}

void
TermListener::handleClearOwnership(Tsq::Uuid *id)
{
    for (auto term: m_terms)
        if (term->isTerm())
            term->clearOwner(*id);

    delete id;
}

void
TermListener::handleChangeOwnership(OwnershipChange *params)
{
    StringMap saved = params->attributes;

    for (auto term: m_terms)
        if (term->isTerm() && term->changeOwner(params))
            params->attributes = saved;

    delete params;
}

void
TermListener::handleAddTask(TaskBase *task)
{
    if (interrupted()) {
        delete task;
        return;
    }

    bool ok = true;
    {
        Lock lock(this);

        if (task->exclusive()) {
            if (m_taskTargets.count(task->targetName()))
                ok = false;
            else
                m_taskTargets.insert(task->targetName());
        }
        if (ok) {
            if (m_taskMap.count(task->taskId()))
                ok = false;
            else
                m_taskMap[task->taskId()] = task;
        }
    }

    if (ok) {
        task->start(-1);
    } else {
        task->reportDuplicateTask();
        delete task;
    }
}

bool
TermListener::handleRemoveTask(TaskBase *task)
{
    {
        Lock lock(this);

        if (task->exclusive())
            m_taskTargets.erase(task->targetName());

        m_taskMap.erase(task->taskId());
    }

    task->join();
    delete task;

    return checkCloseConditions();
}

bool
TermListener::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case ListenerAddTerm:
        handleAddTerm((ConnInstance*)item.value);
        break;
    case ListenerAddConn:
        handleAddConn((RawInstance*)item.value);
        break;
    case ListenerConfirmTerm:
        handleConfirmTerm((ConnInstance*)item.value);
        break;
    case ListenerRemoveTerm:
        return handleRemoveTerm((ConnInstance*)item.value);
    case ListenerAddReader:
        addReader(item.value, item.value, StringMap());
        break;
    case ListenerConfirmReader:
        handleConfirmReader((TermReader*)item.value);
        break;
    case ListenerRemoveReader:
        return handleRemoveReader((TermReader*)item.value);
    case ListenerAddServer:
        handleAddServer((ServerProxy*)item.value);
        break;
    case ListenerRemoveServer:
        handleRemoveServer((ServerProxy*)item.value, item.value2);
        break;
    case ListenerAddProxy:
        handleAddProxy((TermProxy*)item.value);
        break;
    case ListenerRemoveProxy:
        handleRemoveProxy((TermProxy*)item.value, item.value2);
        break;
    case ListenerWatchReleased:
        handleWatchReleased((BaseWatch*)item.value);
        break;
    case ListenerClearOwnership:
        handleClearOwnership((Tsq::Uuid*)item.value);
        break;
    case ListenerChangeOwnership:
        handleChangeOwnership((OwnershipChange*)item.value);
        break;
    case ListenerAddTask:
        handleAddTask((TaskBase*)item.value);
        break;
    case ListenerRemoveTask:
        return handleRemoveTask((TaskBase*)item.value);
    default:
        break;
    }

    return true;
}

bool
TermListener::handleInterrupt()
{
    LOGDBG("Listener: starting shutdown\n");
    closefd();

    // tell everybody to start packing up
    for (auto &task: m_taskMap)
        task.second->stop(TSQ_STATUS_SERVER_SHUTDOWN);
    for (auto reader: m_readers)
        reader->stop(TSQ_STATUS_SERVER_SHUTDOWN);
    for (auto term: m_terms)
        term->stop(TSQ_STATUS_SERVER_SHUTDOWN);

    return checkCloseConditions();
}

void
TermListener::threadMain()
{
    g_reaper->start(-1);
    g_monitor->start(-1);

    loadfd();
    m_fds.emplace_back(pollfd{ .fd = s_reloadFd[0], .events = POLLIN });

    try {
        if (m_initialrd != -1) {
            osMakeNonblocking(m_initialrd);
            osMakeNonblocking(m_initialwd);
            addReader(m_initialrd, m_initialwd, m_environ);
        }
        runDescriptorLoopMulti();
    }
    catch (const TsqException &e) {
        LOGERR("Listener: %s\n", e.what());
        m_exitStatus = 1;
    } catch (const std::exception &e) {
        LOGERR("Listener: caught exception: %s\n", e.what());
        m_exitStatus = 1;
    }

    g_reaper->stop(TSQ_STATUS_SERVER_SHUTDOWN);
    g_monitor->stop(TSQ_STATUS_SERVER_SHUTDOWN);
    g_monitor->join();

    if (s_deathSignal)
        LOGDBG("Listener: exiting on signal %d\n", s_deathSignal);
    else
        LOGDBG("Listener: exiting\n");

    closefd();
}
