// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/exception.h"
#include "app/logging.h"
#include "listener.h"
#include "conn.h"
#include "term.h"
#include "buffers.h"
#include "screen.h"
#include "region.h"
#include "filetracker.h"
#include "task.h"
#include "taskmodel.h"
#include "manager.h"
#include "stack.h"
#include "settings/settings.h"
#include "settings/profile.h"
#include "settings/connect.h"
#include "os/time.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/sequences.h"
#include "lib/attrstr.h"
#include "lib/trstr.h"
#include "config.h"

/*
 * Push
 */
void
ServerConnection::pushClientAnnounce()
{
    if (m_announced)
        return;

    if (m_independent) {
        // Channel test
        if (m_raw) {
            Tsq::ProtocolMarshaler m(TSQ_DISCARD);
            m.addBytes(TSQ_CHANNEL_TEST, sizeof(TSQ_CHANNEL_TEST));
            send(m.resultPtr(), m.length());
        }

        // Configure keepalives
        if (m_keepalive) {
            Tsq::ProtocolMarshaler m(TSQ_CONFIGURE_KEEPALIVE);
            m.addNumber(m_keepalive);
            m_timerId = startTimer(m_keepalive * 2);
            send(m.resultPtr(), m.length());
        }
    }

    // Client announce
    Tsq::ProtocolMarshaler m(TSQ_ANNOUNCE_CLIENT, 16, g_listener->id().buf);
    m.addNumber(CLIENT_VERSION);
    m.addNumberPair(0, Tsq::TakeOwnership);

    for (const auto &i: g_listener->attributes()) {
        m.addStringPair(i.first, i.second);
    }

    push(m.resultPtr(), m.length());
    m_announced = true;
}

/*
 * Server Push
 */
static inline void
push(ServerInstance *server, const std::string &data)
{
    ServerConnection *conn = server->conn();
    if (conn)
        conn->push(data.data(), data.size());
}

void
TermListener::pushServerAttribute(ServerInstance *server, const QString &key, const QString &value)
{
    Tsq::ProtocolMarshaler m(TSQ_SET_SERVER_ATTRIBUTE);
    m.addUuidPair(server->id(), m_id);
    m.addStringPair(key.toStdString(), value.toStdString());

    push(server, m.result());
}

void
TermListener::pushServerAttributeRemove(ServerInstance *server, const QString &key)
{
    Tsq::ProtocolMarshaler m(TSQ_REMOVE_SERVER_ATTRIBUTE);
    m.addUuidPair(server->id(), m_id);
    m.addString(key.toStdString());

    push(server, m.result());
}

void
TermListener::pushServerMonitorInput(ServerInstance *server, const QByteArray &data)
{
    Tsq::ProtocolMarshaler m(TSQ_MONITOR_INPUT);
    m.addUuidPair(server->id(), m_id);
    m.addBytes(data.data(), data.size());

    push(server, m.result());
}

void
TermListener::pushServerAvatarRequest(ServerInstance *server, const Tsq::Uuid &id)
{
    Tsq::ProtocolMarshaler m(TSQ_GET_CLIENT_ATTRIBUTE);
    m.addUuidPair(server->id(), m_id);
    m.addUuid(id);
    m.addBytes(Tsq::attr_AVATAR);

    push(server, m.result());
}

/*
 * Term Push
 */
static inline void
push(TermInstance *term, const std::string &data)
{
    ServerConnection *conn = term->server()->conn();
    if (conn)
        conn->push(data.data(), data.size());
}

void
TermListener::pushTermCreate(TermInstance *term, QSize size)
{
    Tsq::ProtocolMarshaler m(TSQ_CREATE_TERM);
    m.addUuidPair(term->server()->id(), m_id);
    m.addUuid(term->id());
    m.addNumberPair(size.width(), size.height());

    for (auto i = term->attributes().cbegin(), j = term->attributes().cend(); i != j; ++i) {
        m.addStringPair(i.key().toStdString(), i.value().toStdString());
    }

    push(term, m.result());
}

void
TermListener::pushTermDuplicate(TermInstance *term, QSize size, const Tsq::Uuid &source)
{
    Tsq::ProtocolMarshaler m(TSQ_DUPLICATE_TERM);
    m.addUuidPair(source, m_id);
    m.addUuid(term->id());
    m.addNumberPair(size.width(), size.height());

    for (auto i = term->attributes().cbegin(), j = term->attributes().cend(); i != j; ++i) {
        m.addStringPair(i.key().toStdString(), i.value().toStdString());
    }

    push(term, m.result());
}

void
TermListener::pushTermAttribute(TermInstance *term, const QString &key, const QString &value)
{
    Tsq::ProtocolMarshaler m(TSQ_SET_TERM_ATTRIBUTE);
    m.addUuidPair(term->id(), m_id);
    m.addStringPair(key.toStdString(), value.toStdString());

    push(term, m.result());
}

void
TermListener::pushTermAttributes(TermInstance *term, const AttributeMap &map)
{
    Tsq::ProtocolMarshaler m(TSQ_SET_TERM_ATTRIBUTE);
    m.addUuidPair(term->id(), m_id);

    for (auto i = map.cbegin(), j = map.cend(); i != j; ++i) {
        m.addStringPair(i.key().toStdString(), i.value().toStdString());
    }

    push(term, m.result());
}

void
TermListener::pushTermAttributeRemove(TermInstance *term, const QString &key)
{
    Tsq::ProtocolMarshaler m(TSQ_REMOVE_TERM_ATTRIBUTE);
    m.addUuidPair(term->id(), m_id);
    m.addString(key.toStdString());

    push(term, m.result());
}

void
TermListener::pushTermRemove(TermInstance *term)
{
    Tsq::ProtocolMarshaler m(TSQ_REMOVE_TERM);
    m.addUuidPair(term->id(), m_id);

    push(term, m.result());
}

void
TermListener::pushTermDisconnect(TermInstance *term)
{
    Tsq::ProtocolMarshaler m(TSQ_REQUEST_DISCONNECT);
    m.addUuidPair(term->id(), m_id);

    push(term, m.result());
}

void
TermListener::pushTermInput(TermInstance *term, const QByteArray &data)
{
    Tsq::ProtocolMarshaler m(TSQ_INPUT);
    m.addUuidPair(term->id(), m_id);
    m.addBytes(data.data(), data.size());

    push(term, m.result());

    if (term == m_inputLeader) {
        auto &str = m.result();
        for (auto follower: m_inputFollowers) {
            str.replace(8, 16, follower->id().buf, 16);
            push(follower, str);
        }
    }
}

void
TermListener::pushTermMouseEvent(TermInstance *term, uint64_t flags, int x, int y)
{
    Tsq::ProtocolMarshaler m(TSQ_MOUSE_INPUT);
    m.addUuidPair(term->id(), m_id);
    m.addNumber(flags);
    m.addNumberPair(x, y);

    push(term, m.result());
}

void
TermListener::pushTermResize(TermInstance *term, QSize size)
{
    Tsq::ProtocolMarshaler m(TSQ_RESIZE_TERM);
    m.addUuidPair(term->id(), m_id);
    m.addNumberPair(size.width(), size.height());

    push(term, m.result());
}

void
TermListener::pushTermFetch(TermInstance *term, index_t start, index_t end, uint8_t bufid)
{
    Tsq::ProtocolMarshaler m(TSQ_ROW_CONTENT);
    m.addUuidPair(term->id(), m_id);
    m.addNumber64(start);
    m.addNumber64(end);
    m.addNumber(bufid);

    push(term, m.result());
}

void
TermListener::pushTermGetImage(TermInstance *term, contentid_t id)
{
    Tsq::ProtocolMarshaler m(TSQ_IMAGE_CONTENT);
    m.addUuidPair(term->id(), m_id);
    m.addNumber64(id);

    push(term, m.result());
}

void
TermListener::pushTermScrollLock(TermInstance *term)
{
    Tsq::ProtocolMarshaler m(TSQ_TOGGLE_SOFT_SCROLL_LOCK);
    m.addUuidPair(term->id(), m_id);

    push(term, m.result());
}

void
TermListener::pushTermOwnership(TermInstance *term)
{
    Tsq::ProtocolMarshaler m(TSQ_CHANGE_OWNER);
    m.addUuidPair(term->id(), m_id);

    push(term, m.result());
}

void
TermListener::pushTermReset(TermInstance *term, unsigned flags)
{
    Tsq::ProtocolMarshaler m(TSQ_RESET_TERM);
    m.addUuidPair(term->id(), m_id);
    m.addNumber(flags);

    push(term, m.result());
}

void
TermListener::pushTermCaporder(TermInstance *term, uint8_t caporder)
{
    Tsq::ProtocolMarshaler m(TSQ_BUFFER_CAPACITY);
    m.addUuidPair(term->id(), m_id);
    m.addNumber(caporder << 8);

    push(term, m.result());
}

void
TermListener::pushTermSignal(TermInstance *term, unsigned signal)
{
    Tsq::ProtocolMarshaler m(TSQ_SEND_SIGNAL);
    m.addUuidPair(term->id(), m_id);
    m.addNumber(signal);

    push(term, m.result());
}

void
TermListener::pushRegionCreate(TermInstance *term, const Region *r)
{
    Tsq::ProtocolMarshaler m(TSQ_CREATE_REGION);
    m.addUuidPair(term->id(), m_id);
    m.addNumberPair(0, 0);
    m.addNumber64(r->startRow);
    m.addNumber64(r->endRow);
    m.addNumberPair(r->startCol, r->endCol);

    for (auto i = r->attributes.cbegin(), j = r->attributes.cend(); i != j; ++i) {
        m.addStringPair(i.key().toStdString(), i.value().toStdString());
    }

    push(term, m.result());
}

void
TermListener::pushRegionRemove(TermInstance *term, regionid_t region)
{
    Tsq::ProtocolMarshaler m(TSQ_REMOVE_REGION);
    m.addUuidPair(term->id(), m_id);
    m.addNumberPair(0, region);

    push(term, m.result());
}

/*
 * Plain commands
 */
void
ServerConnection::wireServerAnnounce(Tsq::ProtocolUnmarshaler &unm)
{
    Tsq::Uuid serverId = unm.parseUuid();
    Tsq::Uuid termId = unm.parseUuid();
    unsigned version = unm.parseNumber();
    unsigned hops = unm.parseNumber();
    unsigned nTerms = unm.parseNumber();

    TermInstance *term = m_terms.value(termId);
    ServerInstance *server = g_listener->lookupServer(serverId);

    qCDebug(lcCommand) << "Server" << serverId.str().c_str()
                       << "connected via term" << termId.str().c_str();

    if (server) {
        if (server->conn() == nullptr) {
            // Remove disconnected server
            removeServer(server, false);
        } else {
            qCWarning(lcCommand, "Received duplicate server announcement");
            return;
        }
    }

    if (m_haveServer) {
        server = new ServerInstance(serverId, term, this);
    } else {
        server = this;
        changeId(serverId);
        m_haveServer = true;
    }

    // Do this before setting attributes
    auto *info = g_settings->server(serverId, m_conninfo, server == this);
    server->setServerInfo(info, version, hops);

    while (unm.remainingLength()) {
        uint32_t len;
        const char *str = unm.parseString(&len);

        if (len == 0)
            break;

        QString key = QString::fromUtf8(str, len);
        str = unm.parseString(&len);
        server->setAttribute(key, QString::fromUtf8(str, len));
    }

    if (term) {
        // Note: uses server attributes set above
        term->setPeer(server);
    }

    addServer(server);
    // Note: may emit ready signal
    server->setTermCount(nTerms);
}

void
ServerConnection::wireTermAnnounce(Tsq::ProtocolUnmarshaler &unm, bool isTerm)
{
    Tsq::Uuid termId = unm.parseUuid();
    Tsq::Uuid serverId = unm.parseUuid();
    /* unsigned hops = */ unm.parseNumber();
    QSize size;
    if (isTerm) {
        size.setWidth(unm.parseNumber());
        size.setHeight(unm.parseNumber());
    }
    ServerInstance *server = m_servers.value(serverId);
    TermInstance *term = g_listener->lookupTerm(termId);
    AttributeMap attributes;

    qCDebug(lcCommand) << "Terminal" << termId.str().c_str()
                       << "connected via server" << serverId.str().c_str();

    while (unm.remainingLength()) {
        uint32_t len;
        const char *str = unm.parseString(&len);

        if (len == 0)
            break;

        QString key = QString::fromUtf8(str, len);
        str = unm.parseString(&len);
        attributes[key] = QString::fromUtf8(str, len);
    }

    if (term && term->server()->conn() == nullptr) {
        // Remove disconnected term
        removeTerm(term, false);
        term = nullptr;
    }
    else if (term && (term->registered() || term->server()->conn() != this)) {
        qCWarning(lcCommand, "Received duplicate terminal announcement");
        return;
    }
    else if (!server) {
        qCCritical(lcCommand, "Received terminal announcement on an unknown server");
        return;
    }

    if (term) {
        term->setRegistered();

        for (auto i = attributes.cbegin(), j = attributes.cend(); i != j; ++i)
            term->setAttribute(i.key(), i.value());

        return;
    }

    ProfileSettings *profile;

    if (isTerm) {
        QString profileName = attributes.value(g_attr_PROFILE);
        bool haveProfile = g_settings->haveProfile(profileName);
        profile = g_settings->profile(haveProfile ? profileName : g_mtstr);
    }
    else {
        profile = g_settings->defaultProfile();
        auto *manager = g_listener->activeManager();
        size = (manager && manager->activeStack()) ?
            manager->activeStack()->calculateTermSize(profile) :
            profile->termSize();
    }

    term = new TermInstance(termId, server, profile, size, isTerm, false);

    for (auto i = attributes.cbegin(), j = attributes.cend(); i != j; ++i)
        term->setAttribute(i.key(), i.value());

    addTerm(term);
}

static QString
wireDisconnectStr(unsigned reason)
{
    reason &= 0xffff;
    switch (reason) {
    case TSQ_STATUS_SERVER_SHUTDOWN:
        return TR_EXIT2;
    case TSQ_STATUS_FORWARDER_SHUTDOWN:
        return TR_EXIT3;
    case TSQ_STATUS_SERVER_ERROR:
        return TR_EXIT4;
    case TSQ_STATUS_FORWARDER_ERROR:
        return TR_EXIT5;
    case TSQ_STATUS_PROTOCOL_ERROR:
        return TR_EXIT7;
    case TSQ_STATUS_DUPLICATE_CONN:
        return TR_EXIT8;
    case TSQ_STATUS_LOST_CONN:
        return TR_EXIT9;
    case TSQ_STATUS_CONN_LIMIT_REACHED:
        return TR_EXIT10;
    case TSQ_STATUS_IDLE_TIMEOUT:
        return TR_EXIT11;
    default:
        return L("Unexpected disconnect code %1").arg(reason);
    }
}

[[noreturn]] static inline void
wireDisconnect(unsigned reason)
{
    throw StringException(wireDisconnectStr(reason));
}

void
ServerConnection::wirePlainCommand(uint32_t command, Tsq::ProtocolUnmarshaler &unm)
{
    switch (command) {
    case TSQ_HANDSHAKE_COMPLETE:
        pushClientAnnounce();
        break;
    case TSQ_ANNOUNCE_SERVER:
        wireServerAnnounce(unm);
        break;
    case TSQ_ANNOUNCE_TERM:
        wireTermAnnounce(unm, true);
        break;
    case TSQ_ANNOUNCE_CONN:
        wireTermAnnounce(unm, false);
        break;
    case TSQ_DISCONNECT:
        wireDisconnect(unm.parseOptionalNumber(TSQ_STATUS_FORWARDER_ERROR));
        break;
    case TSQ_KEEPALIVE:
        push(TSQ_RAW_KEEPALIVE, TSQ_RAW_KEEPALIVE_LEN);
        break;
    default:
        qCWarning(lcCommand, "Unrecognized plain command %x", command);
        break;
    }
}

/*
 * Server commands
 */
void
ServerConnection::wireServerAttribute(ServerInstance *server, Tsq::ProtocolUnmarshaler &unm)
{
    uint32_t len;
    const char *str = unm.parseString(&len);
    const size_t n = sizeof(TSQT_ATTR_PREFIX) - 1;

    if (len < n || strncmp(str, TSQT_ATTR_PREFIX, n)) {
        QString key = QString::fromUtf8(str, len);

        if (unm.remainingLength()) {
            str = unm.parseString(&len);
            server->setAttribute(key, QString::fromUtf8(str, len));
        } else {
            server->removeAttribute(key);
        }
    }
}

void
ServerConnection::wireServerRemove(ServerInstance *server, Tsq::ProtocolUnmarshaler &unm)
{
    unsigned reason = unm.parseNumber();
    bool cutoff = (reason & TSQ_FLAG_PROXY_CLOSED) &&
        (reason & 0xffff) > TSQ_STATUS_CLOSED;

    qCDebug(lcCommand) << "Server" << server->id().str().c_str() <<
        "disconnect code" << reason;

    if (server == this) {
        qCCritical(lcCommand, "Received server removal for a local connection");
    } else if (cutoff) {
        stopServer(server);
    } else {
        removeServer(server, true);
    }
}

void
ServerConnection::wireTaskOutput(ServerInstance *server, Tsq::ProtocolUnmarshaler &unm)
{
    Tsq::Uuid taskId = unm.parseUuid();
    TermTask *task = g_listener->taskmodel()->getTask(taskId);

    // Ignore tasks we don't know about
    if (!task) {
        qCDebug(lcCommand) << "Unknown task for task output";
    }
    else if (task->server() != server) {
        qCWarning(lcCommand) << "Bad server in task output message";
    }
    else {
        task->handleOutput(&unm);
    }

    m_timestamp = osMonotime();
}

void
ServerConnection::wireTaskQuestion(ServerInstance *server, Tsq::ProtocolUnmarshaler &unm)
{
    Tsq::Uuid taskId = unm.parseUuid();
    TermTask *task = g_listener->taskmodel()->getTask(taskId);

    // Ignore tasks we don't know about
    if (!task) {
        qCDebug(lcCommand) << "Unknown task for task question";
    }
    else if (task->server() != server) {
        qCWarning(lcCommand) << "Bad server in task question message";
    }
    else {
        task->handleQuestion(unm.parseNumber());
    }

    m_timestamp = osMonotime();
}

void
ServerConnection::wireClientAttribute(Tsq::ProtocolUnmarshaler &unm)
{
    Tsq::Uuid id = unm.parseUuid();
    if (unm.parseString() == Tsq::attr_AVATAR)
        g_settings->parseAvatar(id, unm.remainingBytes(), unm.remainingLength());
}

void
ServerConnection::wireServerCommand(uint32_t command, Tsq::ProtocolUnmarshaler &unm)
{
    Tsq::Uuid id = unm.parseUuid();
    ServerInstance *server = m_servers.value(id);

    // Ignore servers we don't know about
    if (!server) {
        qCDebug(lcCommand) << "Unknown target for server command" << command;
        return;
    }

    switch (command) {
    case TSQ_GET_SERVER_ATTRIBUTE:
        wireServerAttribute(server, unm);
        break;
    case TSQ_REMOVE_SERVER:
        wireServerRemove(server, unm);
        break;
    case TSQ_TASK_OUTPUT:
        wireTaskOutput(server, unm);
        break;
    case TSQ_TASK_QUESTION:
        wireTaskQuestion(server, unm);
        break;
    case TSQ_GET_CLIENT_ATTRIBUTE_RESPONSE:
        wireClientAttribute(unm);
        break;
    default:
        qCWarning(lcCommand, "Unrecognized server command %x", command);
        break;
    }
}

/*
 * Term commands
 */
inline void
ServerConnection::wireTermBufferCapacity(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    uint64_t rows = unm.parseNumber64();
    unsigned spec = unm.parseNumber();

    term->buffers()->changeCapacity(spec & 0xff, rows, spec >> 8 & 0xff);
}

inline void
ServerConnection::wireTermBufferLength(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    uint64_t rows = unm.parseNumber64();
    uint8_t bufid = unm.parseNumber();

    term->buffers()->changeLength(bufid, rows);
}

inline void
ServerConnection::wireTermSizeChanged(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    int width = unm.parseNumber();
    int height = unm.parseNumber();

    term->setRemoteSize(QSize(width, height));
}

inline void
ServerConnection::wireTermCursorMoved(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    /* int x = */ unm.parseNumber();
    int y = unm.parseNumber();
    int pos = unm.parseNumber();

    term->screen()->setCursor(QPoint(pos, y));
}

inline void
ServerConnection::wireTermMouseMoved(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    int x = unm.parseNumber();
    int y = unm.parseNumber();

    term->setMousePos(QPoint(x, y));
}

inline void
ServerConnection::wireTermImageContent(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    QString id = QString::number(unm.parseNumber64());

    term->updateImage(id, unm.remainingBytes(), unm.remainingLength());
}

inline void
ServerConnection::wireTermBellRang(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    if (unm.parseNumber() == 0)
        term->reportBellRang();
}

void
ServerConnection::wireTermRowChanged(TermInstance *term, Tsq::ProtocolUnmarshaler &unm, bool pushed)
{
    index_t row = unm.parseNumber64();
    const char *ptr = unm.remainingBytes();
    const uint32_t *data = reinterpret_cast<const uint32_t*>(ptr);
    unsigned len = 12;

    if (unm.remainingLength() < len) {
        throw TsqException("Unmarshal failed: invalid row output");
    }

    len += 24 * le32toh(data[2]);

    if (unm.remainingLength() < len) {
        throw TsqException("Unmarshal failed: invalid row output");
    }

    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    for (unsigned z = 0; z < len / 4; ++z)
        const_cast<uint32_t*>(data)[z] = __builtin_bswap32(data[z]);
    #endif

    term->buffers()->setRow(row, data, ptr + len, unm.remainingLength() - len, pushed);
}

void
ServerConnection::wireTermRegionChanged(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    const char *ptr = unm.parseBytes(40);
    const uint32_t *data = reinterpret_cast<const uint32_t*>(ptr);
    AttributeMap attributes;

    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    for (unsigned z = 0; z < 10; ++z)
        const_cast<uint32_t*>(data)[z] = __builtin_bswap32(data[z]);
    #endif

    while (unm.remainingLength()) {
        uint32_t len;
        const char *str = unm.parseString(&len);
        const size_t n = sizeof(TSQT_ATTR_PREFIX) - 1;

        if (len == 0)
            break;
        if (len < n || strncmp(str, TSQT_ATTR_PREFIX, n)) {
            QString key = QString::fromUtf8(str, len);
            str = unm.parseString(&len);
            attributes.insert(key, QString::fromUtf8(str, len));
        }
    }

    term->buffers()->setRegion(data, attributes);
}

inline void
ServerConnection::wireTermDirectoryChanged(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    TermDirectory dir(&unm);
    term->files()->setDirectory(dir);
}

inline void
ServerConnection::wireTermFileChanged(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    TermFile file(term->files()->realdir(), &unm);
    term->files()->updateFile(file);
}

void
ServerConnection::wireTermFileRemoved(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    /* uint64_t mtime = */ unm.parseNumber64();
    QString name = QString::fromStdString(unm.parseString());

    term->files()->removeFile(name);
}

void
ServerConnection::wireTermAttribute(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    uint32_t len;
    const char *str = unm.parseString(&len);
    const size_t n = sizeof(TSQT_ATTR_PREFIX) - 1;

    if (len < n || strncmp(str, TSQT_ATTR_PREFIX, n)) {
        QString key = QString::fromUtf8(str, len);

        if (unm.remainingLength()) {
            str = unm.parseString(&len);
            term->setAttribute(key, QString::fromUtf8(str, len));
        } else {
            term->removeAttribute(key);
        }
    }
}

void
ServerConnection::wireTermRemove(TermInstance *term, Tsq::ProtocolUnmarshaler &unm)
{
    unsigned reason = unm.parseNumber();
    bool abnormal = (reason & 0xffff) > TSQ_STATUS_CLOSED;

    qCDebug(lcCommand) << "Terminal" << term->id().str().c_str() <<
        "disconnect code" << reason;

    if (abnormal) {
        if (reason & TSQ_FLAG_PROXY_CLOSED) {
            // do nothing (assume server connection will be cleared)
            return;
        }
        if (term->peer()) {
            g_listener->reportProxyLost(term, wireDisconnectStr(reason));
        }
    }

    removeTerm(term, true);
}

void
ServerConnection::wireTermCommand(uint32_t command, Tsq::ProtocolUnmarshaler &unm)
{
    Tsq::Uuid id = unm.parseUuid();
    TermInstance *term = m_terms.value(id);

    // Ignore terminals we don't know about
    if (!term) {
        qCDebug(lcCommand) << "Unknown target for term command" << command;
        return;
    }

    switch (command) {
    case TSQ_BEGIN_OUTPUT:
        term->beginUpdate();
        term->buffers()->beginUpdate();
        break;
    case TSQ_FLAGS_CHANGED:
        term->setFlags(unm.parseNumber64());
        break;
    case TSQ_BUFFER_CAPACITY:
        wireTermBufferCapacity(term, unm);
        break;
    case TSQ_BUFFER_LENGTH:
        wireTermBufferLength(term, unm);
        break;
    case TSQ_BUFFER_SWITCHED:
        term->buffers()->setActiveBuffer(unm.parseNumber() & 0xff);
        break;
    case TSQ_SIZE_CHANGED:
        wireTermSizeChanged(term, unm);
        break;
    case TSQ_CURSOR_MOVED:
        wireTermCursorMoved(term, unm);
        break;
    case TSQ_BELL_RANG:
        wireTermBellRang(term, unm);
        break;
    case TSQ_ROW_CONTENT:
        wireTermRowChanged(term, unm, true);
        break;
    case TSQ_ROW_CONTENT_RESPONSE:
        wireTermRowChanged(term, unm, false);
        break;
    case TSQ_REGION_UPDATE:
        wireTermRegionChanged(term, unm);
        break;
    case TSQ_DIRECTORY_UPDATE:
        wireTermDirectoryChanged(term, unm);
        break;
    case TSQ_FILE_UPDATE:
        wireTermFileChanged(term, unm);
        break;
    case TSQ_FILE_REMOVED:
        wireTermFileRemoved(term, unm);
        break;
    case TSQ_END_OUTPUT:
        m_timestamp = osMonotime();
        // fallthru
    case TSQ_END_OUTPUT_RESPONSE:
        term->endUpdate();
        term->buffers()->endUpdate();
        break;
    case TSQ_MOUSE_MOVED:
        wireTermMouseMoved(term, unm);
        break;
    case TSQ_IMAGE_CONTENT_RESPONSE:
        wireTermImageContent(term, unm);
        break;
    case TSQ_THROTTLE_RESUME:
        throttleResume(term, true);
        break;
    case TSQ_GET_TERM_ATTRIBUTE:
    case TSQ_GET_CONN_ATTRIBUTE:
        wireTermAttribute(term, unm);
        break;
    case TSQ_REMOVE_TERM:
    case TSQ_REMOVE_CONN:
        wireTermRemove(term, unm);
        break;
    default:
        qCWarning(lcCommand, "Unrecognized term command %x", command);
        break;
    }
}

/*
 * Client commands
 */
void
ServerConnection::wireThrottlePause(Tsq::ProtocolUnmarshaler &unm)
{
    Tsq::Uuid destId = unm.parseUuid();
    Tsq::Uuid hopId = unm.parseUuid();
    // uint64_t actual = unm.parseNumber64();
    // uint64_t limit = unm.parseNumber64();
    TermInstance *hop = m_terms.value(hopId);

    if (hop) {
        IdBase *dest;

        if ((dest = m_terms.value(destId)) || (dest = m_servers.value(destId))) {
            dest->setThrottled(true);
            m_throttles[dest].insert(hop);
        }
    }
}

void
ServerConnection::wireClientCommand(uint32_t command, Tsq::ProtocolUnmarshaler &unm)
{
    // Client id
    unm.parseBytes(16);

    switch (command) {
    case TSQ_THROTTLE_PAUSE:
        wireThrottlePause(unm);
        break;
    case TSQ_BEGIN_OUTPUT_RESPONSE:
        wireTermCommand(TSQ_BEGIN_OUTPUT, unm);
        break;
    case TSQ_REGION_UPDATE_RESPONSE:
        wireTermCommand(TSQ_REGION_UPDATE, unm);
        break;
    case TSQ_ROW_CONTENT_RESPONSE:
    case TSQ_END_OUTPUT_RESPONSE:
    case TSQ_IMAGE_CONTENT_RESPONSE:
        wireTermCommand(command, unm);
        break;
    case TSQ_TASK_OUTPUT:
    case TSQ_TASK_QUESTION:
    case TSQ_GET_CLIENT_ATTRIBUTE_RESPONSE:
        wireServerCommand(command, unm);
        break;
    default:
        qCWarning(lcCommand, "Unrecognized client command %x", command);
        break;
    }
}

bool
ServerConnection::protocolCallback(uint32_t command, uint32_t length, const char *body)
{
//    qCDebug(lcCommand, "Command %x, length %u", command, length);

    Tsq::ProtocolUnmarshaler unm(body, length);

    switch (command & TSQ_CMDTYPE_MASK) {
    case TSQ_CMDTYPE_PLAIN:
        wirePlainCommand(command, unm);
        break;
    case TSQ_CMDTYPE_SERVER:
        wireServerCommand(command, unm);
        break;
    case TSQ_CMDTYPE_TERM:
        wireTermCommand(command, unm);
        break;
    case TSQ_CMDTYPE_CLIENT:
        wireClientCommand(command, unm);
        break;
    default:
        qCWarning(lcCommand, "Unrecognized command %x", command);
        break;
    }

    return true;
}
