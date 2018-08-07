// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "conn.h"
#include "output.h"
#include "basewatch.h"
#include "serverproxy.h"
#include "termproxy.h"
#include "listener.h"
#include "os/logging.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/sequences.h"
#include "lib/attr.h"
#include "lib/attrstr.h"
#include "config.h"

#include <cassert>

ConnInstance::ConnInstance(const char *name, bool isTerm) :
    AttributeBase(name),
    m_output(new TermOutput(this)),
    m_isTerm(isTerm)
{
}

ConnInstance::~ConnInstance()
{
    assert(!m_haveConnection);
    assert(m_watches.empty());
    assert(m_knownServers.empty());
    assert(m_knownTerms.empty());

    delete m_machine;
    delete m_output;
}

/*
 * Other threads
 */
bool
ConnInstance::addWatch(BaseWatch *watch)
{
    FlexLock lock(this);

    if (m_closing) {
        return false;
    } else {
        m_watches.insert(watch);
        sendWorkAndUnlock(lock, TermWatchAdded, watch);
        return true;
    }
}

void
ConnInstance::doSetOwner(const Tsq::Uuid &owner, OwnershipInfo *oi)
{
    // Lock assumed to be held
    m_sender = m_owner = owner;
    StringMap changes;

    {
        StateLock slock(this, true);

        for (auto i = m_attributes.begin(); i != m_attributes.end(); ) {
            if (i->first.compare(0, sizeof(TSQ_ATTR_OWNER_PREFIX) - 1,
                                 Tsq::attr_OWNER_PREFIX) &&
                i->first.compare(0, sizeof(TSQ_ATTR_SENDER_PREFIX) - 1,
                                 Tsq::attr_SENDER_PREFIX))
            {
                ++i;
                continue;
            }

            std::string &spec = i->second;
            spec.assign(i->first);
            spec.push_back('\0');

            auto j = oi->attributes.find(i->first);
            if (j == oi->attributes.end()) {
                changes[i->first] = std::move(spec);
                i = m_attributes.erase(i);
            } else {
                spec.append(j->second);
                spec.push_back('\0');
                changes[i->first] = std::move(spec);
                i->second = std::move(j->second);
                oi->attributes.erase(j);
                ++i;
            }
        }

        for (auto &&i: oi->attributes) {
            std::string spec = i.first;
            spec.push_back('\0');
            spec.append(i.second);
            spec.push_back('\0');
            changes[i.first] = std::move(spec);
            m_attributes[i.first] = std::move(i.second);
        }
    }

    for (auto watch: m_watches) {
        // two locks held
        watch->pushAttributeChanges(changes);
    }

    stageWork(TermUpdateEnviron, new SharedStringMap(oi->environ));
    commitWork();
}

void
ConnInstance::doSetSender(const Tsq::Uuid &sender, StringMap &map)
{
    // Lock assumed to be held
    m_sender = sender;
    StringMap changes;

    {
        StateLock slock(this, true);

        for (auto i = m_attributes.begin(); i != m_attributes.end(); ) {
            if (i->first.compare(0, sizeof(TSQ_ATTR_SENDER_PREFIX) - 1,
                                 Tsq::attr_SENDER_PREFIX))
            {
                ++i;
                continue;
            }

            std::string &spec = i->second;
            spec.assign(i->first);
            spec.push_back('\0');

            auto j = map.find(i->first);
            if (j == map.end()) {
                changes[i->first] = std::move(spec);
                i = m_attributes.erase(i);
            } else {
                spec.append(j->second);
                spec.push_back('\0');
                changes[i->first] = std::move(spec);
                i->second = std::move(j->second);
                map.erase(j);
                ++i;
            }
        }

        for (auto &&i: map) {
            std::string spec = i.first;
            spec.push_back('\0');
            spec.append(i.second);
            spec.push_back('\0');
            changes[i.first] = std::move(spec);
            m_attributes[i.first] = std::move(i.second);
        }
    }

    for (auto watch: m_watches) {
        // two locks held
        watch->pushAttributeChanges(changes);
    }
}

bool
ConnInstance::testOwner(const Tsq::Uuid &owner)
{
    if (m_isTerm) {
        Lock lock(this);

        if (!m_owner) {
            OwnershipInfo oi;
            g_listener->getOwnerAttributes(owner, oi);
            doSetOwner(owner, &oi);
            return true;
        } else {
            return (m_owner == owner);
        }
    }
    return false;
}

bool
ConnInstance::testSender(const Tsq::Uuid &owner)
{
    if (m_isTerm) {
        Lock lock(this);

        if (!m_owner) {
            OwnershipInfo oi;
            g_listener->getOwnerAttributes(owner, oi);
            doSetOwner(owner, &oi);
        } else if (m_owner != owner && !testAttribute(Tsq::attr_PREF_INPUT)) {
            return false;
        } else if (m_sender != owner) {
            StringMap map;
            g_listener->getSenderAttributes(owner, map);
            doSetSender(owner, map);
        }
        return true;
    }
    return false;
}

void
ConnInstance::setOwner(const Tsq::Uuid &owner)
{
    if (m_isTerm) {
        Lock lock(this);

        if (m_owner != owner) {
            OwnershipInfo oi;
            g_listener->getOwnerAttributes(owner, oi);
            doSetOwner(owner, &oi);
        }
    }
}

bool
ConnInstance::changeOwner(OwnershipChange *params)
{
    Lock lock(this);

    if (m_owner == params->oldId) {
        doSetOwner(params->newId, params);
        return true;
    }

    return false;
}

void
ConnInstance::clearOwner(const Tsq::Uuid &owner)
{
    Lock lock(this);

    if (m_owner == owner) {
        m_owner = Tsq::Uuid();
        std::string value = m_owner.str();

        {
            StateLock slock(this, true);

            auto i = m_attributes.find(Tsq::attr_OWNER_ID);
            if (i == m_attributes.end()) {
                m_attributes[Tsq::attr_OWNER_ID] = value;
            } else {
                i->second = value;
            }
        }

        std::string spec = Tsq::attr_OWNER_ID;
        spec.push_back('\0');
        spec.append(value);
        spec.push_back('\0');

        for (auto watch: m_watches) {
            // two locks held
            watch->pushAttributeChange(Tsq::attr_OWNER_ID, spec);
        }
    }
}

/*
 * This thread
 */
void
ConnInstance::closefd()
{
    if (m_fd != -1) {
        if (m_output->started()) {
            LOGDBG("Term %p: waiting for output %p\n", this, m_output);
            m_output->stop(0);
            m_output->join();
        }

        ThreadBase::closefd();
    }
}

inline bool
ConnInstance::checkCloseConditions(bool noWatches)
{
    // Three conditions for close:
    // 1. asked to close (i.e. handleClose called)
    // 2. all watches removed (determining this requires a lock)
    // 3. no outstanding connection (meaning: no server proxies)

    return !m_closing || !noWatches || m_haveConnection;
}

bool
ConnInstance::handleClose(unsigned flags, int reason)
{
    {
        Lock lock(this);

        if (m_closing)
            return true;
        else
            m_closing = true;
    }

    // past this point, OK to access m_watches freely

    disconnect(flags, reason);
    closefd();

    if (m_haveExitStatus)
        reason = m_exitStatus;

    // ask readers nicely to release their watches
    for (auto watch: m_watches)
        watch->requestRelease(reason);

    return checkCloseConditions(m_watches.empty());
}

bool
ConnInstance::handleWatchReleased(BaseWatch *watch)
{
    bool noWatches;

    {
        Lock lock(this);

        m_watches.erase(watch);
        noWatches = m_watches.empty();
    }

    watch->teardown();
    delete watch;

    return checkCloseConditions(noWatches);
}

bool
ConnInstance::handleServerReleased(ServerProxy *proxy)
{
    m_knownServers.erase(proxy->id());
    m_removingServers.erase(proxy);
    assert(proxy->nTerms() == 0);
    delete proxy;

    if (m_removingConnection && m_removingServers.empty()) {
        m_knownServers.clear();
        m_ignoredServers.clear();
        m_knownTerms.clear();
        m_ignoredTerms.clear();

        m_haveConnection = m_removingConnection = false;

        if (m_reportDisconnect)
            postDisconnect(false);
    }

    bool noWatches;
    {
        Lock lock(this);
        noWatches = m_watches.empty();
    }
    return checkCloseConditions(noWatches);
}

void
ConnInstance::handleProxyReleased(TermProxy *proxy)
{
    m_knownTerms.erase(proxy->id());
    m_removingTerms.erase(proxy);
    delete proxy;
}

/*
 * Push
 */
void
ConnInstance::disconnect(unsigned flags, int reason)
{
    if (m_haveConnection && !m_removingConnection) {
        Tsq::ProtocolMarshaler m;

        if (flags & DisActive) {
            m.begin(TSQ_DISCONNECT);
            m.addNumber(reason);
            m_output->submitCommand(std::move(m.result()));
        }

        for (const auto &i: m_activeTerms) {
            g_listener->unregisterProxy(i.first, i.second, reason);
            m_removingTerms.insert(i.second);
        }
        for (const auto &i: m_activeServers) {
            g_listener->unregisterServer(i.first, i.second, reason);
            m_removingServers.insert(i.second);
        }

        m_activeTerms.clear();
        m_activeServers.clear();
        m_machine->reset();

        if (m_removingServers.empty()) {
            assert(m_removingTerms.empty());

            m_knownServers.clear();
            m_ignoredServers.clear();
            m_knownTerms.clear();
            m_ignoredTerms.clear();

            m_haveConnection = false;

            if (flags & DisReport)
                postDisconnect(flags & DisLocked);
        } else {
            m_removingConnection = true;
            m_reportDisconnect = flags & DisReport;
        }
    }
}

inline void
ConnInstance::pushTaskPause(const char *body)
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_PAUSE);
    m.addUuidPairReversed(body);
    m.addUuidPair(body + 32, g_listener->id());
    m_output->submitCommand(std::move(m.result()));
}

inline void
ConnInstance::pushKeepalive()
{
    std::string keepalive(TSQ_RAW_KEEPALIVE, TSQ_RAW_KEEPALIVE_LEN);
    m_output->submitCommand(std::move(keepalive));
}

void
ConnInstance::pushChannelTest()
{
    Tsq::ProtocolMarshaler m(TSQ_DISCARD);
    m.addBytes(TSQ_CHANNEL_TEST, sizeof(TSQ_CHANNEL_TEST));
    m_output->submitCommand(std::move(m.result()));
}

void
ConnInstance::pushConfigureKeepalive(unsigned timeout)
{
    Tsq::ProtocolMarshaler m(TSQ_CONFIGURE_KEEPALIVE);
    m.addNumber(timeout);
    m_output->submitCommand(std::move(m.result()));
}

void
ConnInstance::pushThrottleResume(const Tsq::Uuid &id)
{
    Tsq::ProtocolMarshaler m(TSQ_THROTTLE_RESUME, 16, id.buf);
    g_listener->forwardToClients(m.result());
}

/*
 * Plain commands
 */
void
ConnInstance::wireServerAnnounce(const char *body, uint32_t length)
{
    if (length < 40) {
        LOGNOT("Term %p: undersize message %x\n", this, TSQ_ANNOUNCE_SERVER);
        return;
    }

    Tsq::Uuid serverId(body);
    Tsq::Uuid hopId(body + 16);

    if (hopId != m_id && !m_knownTerms.count(hopId)) {
        LOGNOT("Term %p: unknown sender of command %x\n", this, TSQ_ANNOUNCE_SERVER);
        return;
    }

    if (!m_knownServers.count(serverId)) {
        m_knownServers.insert(serverId);

        auto *proxy = new ServerProxy(this, body, length);

        if (g_listener->registerServer(serverId, proxy)) {
            m_activeServers.emplace(serverId, proxy);
        } else {
            m_ignoredServers.insert(serverId);
            delete proxy;
        }
    }
}

void
ConnInstance::wireTermAnnounce(uint32_t command, uint32_t length, const char *body)
{
    if (length < 36) {
        LOGNOT("Term %p: undersize message %x\n", this, command);
        return;
    }

    Tsq::Uuid termId(body);
    Tsq::Uuid hopId(body + 16);

    if (!m_knownServers.count(hopId)) {
        LOGNOT("Term %p: unknown sender of command %x\n", this, command);
        return;
    }

    if (!m_knownTerms.count(termId)) {
        m_knownTerms.insert(termId);

        bool isTerm = (command == TSQ_ANNOUNCE_TERM);
        auto *proxy = new TermProxy(this, body, length, isTerm);

        if (!m_ignoredServers.count(termId) && g_listener->registerProxy(termId, proxy)) {
            m_activeTerms.emplace(termId, proxy);
        } else {
            m_ignoredTerms.insert(termId);
            delete proxy;
        }
    }
}

void
ConnInstance::wireDisconnect(const char *body, uint32_t length)
{
    uint32_t reason = (length >= 4) ?
        le32toh(*(reinterpret_cast<const uint32_t*>(body))) :
        TSQ_STATUS_FORWARDER_ERROR;

    LOGDBG("Term %p: received disconnect code %d\n", this, reason);
    disconnect(DisReport|DisLocked, reason);

    if (!m_isTerm) {
        m_exitStatus = reason;
        m_haveExitStatus = true;
    }
}

bool
ConnInstance::wirePlainCommand(uint32_t command, uint32_t length, const char *body)
{
    switch (command) {
    case TSQ_HANDSHAKE_COMPLETE:
        postConnect();
        return true;
    case TSQ_ANNOUNCE_SERVER:
        wireServerAnnounce(body, length);
        return true;
    case TSQ_ANNOUNCE_TERM:
    case TSQ_ANNOUNCE_CONN:
        wireTermAnnounce(command, length, body);
        return true;
    case TSQ_DISCONNECT:
        wireDisconnect(body, length);
        return false;
    case TSQ_KEEPALIVE:
        pushKeepalive();
        return true;
    default:
        LOGNOT("Term %p: unrecognized command %x\n", this, command);
        return true;
    }
}

/*
 * Server commands
 */
void
ConnInstance::wireServerRemove(const Tsq::Uuid &id, uint32_t length, const char *body)
{
    uint32_t reason = (length >= 4) ?
        le32toh(*(reinterpret_cast<const uint32_t*>(body))) :
        0;

    m_ignoredServers.erase(id);

    auto i = m_activeServers.find(id);
    if (i != m_activeServers.end()) {
        reason ^= TSQ_FLAG_PROXY_CLOSED;
        m_removingServers.insert(i->second);
        m_activeServers.erase(id);
        g_listener->unregisterServer(id, i->second, reason);
    } else {
        m_knownServers.erase(id);
    }
}

bool
ConnInstance::wireServerCommand(uint32_t command, uint32_t length, const char *body)
{
    if (length < 16) {
        LOGNOT("Term %p: undersize message %x\n", this, command);
        return false;
    }

    Tsq::Uuid id(body);
    decltype(m_activeServers)::iterator i;

    if (!m_knownServers.count(id)) {
        LOGNOT("Term %p: unknown recipient for command %x\n", this, command);
        goto out;
    }

    length -= 16;
    body += 16;

    switch (command) {
    case TSQ_REMOVE_SERVER:
        wireServerRemove(id, length, body);
        break;
    default:
        if ((i = m_activeServers.find(id)) != m_activeServers.end()) {
            i->second->wireCommand(command, length, body);
        }
    }
out:
    return true;
}

/*
 * Term commands
 */
void
ConnInstance::wireTermRemove(const Tsq::Uuid &id, uint32_t length, const char *body)
{
    uint32_t reason = (length >= 4) ?
        le32toh(*(reinterpret_cast<const uint32_t*>(body))) :
        0;

    m_ignoredTerms.erase(id);

    auto i = m_activeTerms.find(id);
    if (i != m_activeTerms.end()) {
        reason ^= TSQ_FLAG_PROXY_CLOSED;
        m_removingTerms.insert(i->second);
        m_activeTerms.erase(id);
        g_listener->unregisterProxy(id, i->second, reason);
    } else {
        m_knownTerms.erase(id);
    }
}

bool
ConnInstance::wireTermCommand(uint32_t command, uint32_t length, const char *body)
{
    if (length < 16) {
        LOGNOT("Term %p: undersize message %x\n", this, command);
        return false;
    }

    Tsq::Uuid id(body);
    decltype(m_activeTerms)::iterator i;

    if (!m_knownTerms.count(id)) {
        LOGNOT("Term %p: unknown recipient for command %x\n", this, command);
        goto out;
    }

    length -= 16;
    body += 16;

    switch (command) {
    case TSQ_REMOVE_TERM:
    case TSQ_REMOVE_CONN:
        wireTermRemove(id, length, body);
        break;
    case TSQ_THROTTLE_RESUME:
        pushThrottleResume(id);
        break;
    default:
        if ((i = m_activeTerms.find(id)) != m_activeTerms.end()) {
            i->second->wireCommand(command, length, body);
        }
    }
out:
    return true;
}

/*
 * Client commands
 */
bool
ConnInstance::wireClientCommand(uint32_t command, uint32_t length, const char *body)
{
    if (length < 32) {
        LOGNOT("Term %p: undersize message %x\n", this, command);
        return false;
    }

    Tsq::Uuid id(body + 16);
    Tsq::ProtocolMarshaler m(command, length, body);

    // Forward the message on to the target client
    if (m_ignoredTerms.count(id) || m_ignoredServers.count(id))
        return true;
    if (!m_knownTerms.count(id) && !m_knownServers.count(id)) {
        LOGNOT("Term %p: unknown sender of command %x\n", this, command);
        return true;
    }

    switch (g_listener->forwardToClient(body, m.result())) {
    case 0:
        if (command == TSQ_TASK_OUTPUT && length >= 48)
            pushTaskPause(body);
        break;
    case -1:
        LOGDBG("Term %p: unknown recipient for command %x\n", this, command);
        break;
    }
    return true;
}

bool
ConnInstance::protocolCallback(uint32_t command, uint32_t length, const char *body)
{
    // LOGDBG("Term %p: command %x, length %u\n", this, command, length);

    switch (command & TSQ_CMDTYPE_MASK) {
    case TSQ_CMDTYPE_PLAIN:
        return wirePlainCommand(command, length, body);
    case TSQ_CMDTYPE_SERVER:
        return wireServerCommand(command, length, body);
    case TSQ_CMDTYPE_TERM:
        return wireTermCommand(command, length, body);
    case TSQ_CMDTYPE_CLIENT:
        return wireClientCommand(command, length, body);
    default:
        LOGNOT("Term %p: unrecognized command %x\n", this, command);
        return true;
    }
}

void
ConnInstance::writeFd(const char *buf, size_t len)
{
    assert(m_fd != -1);
    m_output->writeFd(m_fd, buf, len);
}
