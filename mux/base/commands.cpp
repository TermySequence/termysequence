// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "reader.h"
#include "writer.h"
#include "term.h"
#include "output.h"
#include "termwatch.h"
#include "emulator.h"
#include "listener.h"
#include "monitor.h"
#include "downloadtask.h"
#include "uploadtask.h"
#include "misctask.h"
#include "portouttask.h"
#include "portintask.h"
#include "imagetask.h"
#include "runtask.h"
#include "connecttask.h"
#include "mounttask.h"
#include "exception.h"
#include "parsemap.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/attrstr.h"
#include "os/attr.h"
#include "os/time.h"
#include "os/logging.h"

void
TermReader::disconnect()
{
    Tsq::ProtocolMarshaler m;

    for (const auto &id: m_knownClients) {
        if (!m_ignoredClients.count(id)) {
            m.begin(TSQ_REMOVE_CLIENT);
            m.addUuid(id);
            g_listener->unregisterClient(id, m.result());
        }
    }

    m_knownClients.clear();
    m_ignoredClients.clear();
}

/*
 * Push
 */
void
TermReader::pushDisconnect(int reason)
{
    Tsq::ProtocolMarshaler m(TSQ_DISCONNECT);
    m.addNumber(reason);
    m_writer->submitResponse(std::move(m.result()));
}

void
TermReader::pushThrottlePause(const char *body, ConnWatch *watch)
{
    Tsq::ProtocolMarshaler m(TSQ_THROTTLE_PAUSE);
    m.addUuidPairReversed(body);
    m.addUuid(watch->parentId());
    m.addNumber64(watch->parent()->output()->bufferCurrentAmount());
    m.addNumber64(watch->parent()->output()->bufferWarnAmount());
    m_writer->submitResponse(std::move(m.result()));
}

void
TermReader::pushTaskResume(const Tsq::Uuid &id)
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_RESUME, 16, id.buf);
    g_listener->resumeTasks(id);
    g_listener->forwardToServers(m.result());
}

/*
 * Plain commands
 */
bool
TermReader::handlePlainCommand(uint32_t command, uint32_t length, const char *body)
{
    Tsq::ProtocolUnmarshaler unm(body, length);

    switch (command) {
    case TSQ_DISCONNECT:
        m_exitStatus = unm.parseNumber();
        LOGDBG("Reader %p: received disconnect code %d\n", this, m_exitStatus);
        return false;
    case TSQ_KEEPALIVE:
        m_idleOut = false;
        return true;
    case TSQ_CONFIGURE_KEEPALIVE:
        setKeepalive(unm.parseNumber(), 1);
        return true;
    case TSQ_TASK_RESUME:
        pushTaskResume(body);
        return true;
    case TSQ_DISCARD:
        return true;
    default:
        LOGNOT("Reader %p: unrecognized command %x\n", this, command);
        return true;
    }
}

/*
 * Server commands
 */
void
TermReader::commandServerTimeRequest(const char *body)
{
    Tsq::ProtocolMarshaler m(TSQ_GET_SERVER_TIME_RESPONSE);
    m.addUuidPairReversed(body);
    m.addNumber64(osWalltime());
    m_writer->submitResponse(std::move(m.result()));
}

void
TermReader::commandServerGetAttributes(const char *body)
{
    Tsq::ProtocolMarshaler m(TSQ_GET_SERVER_ATTRIBUTES_RESPONSE);
    m.addUuidPairReversed(body);
    m.addBytes(g_listener->commandGetAttributes());
    m_writer->submitResponse(std::move(m.result()));
}

void
TermReader::commandServerGetAttribute(const char *body, uint32_t length)
{
    Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
    Tsq::ProtocolMarshaler m;

    while (unm.remainingLength()) {
        std::string i = unm.parseUtf8();

        if (i.empty())
            break;

        m.begin(TSQ_GET_SERVER_ATTRIBUTE_RESPONSE);
        m.addUuidPairReversed(body);
        m.addBytes(g_listener->commandGetAttribute(i));
        m_writer->submitResponse(std::move(m.result()));
    }
}

void
TermReader::commandServerSetAttribute(const char *body, uint32_t length)
{
    Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
    while (unm.remainingLength()) {
        std::string key = unm.parseUtf8();

        if (key.empty())
            break;
        else if (osRestrictedServerAttribute(key))
            unm.parseUtf8();
        else
            g_listener->commandSetAttribute(key, unm.parseUtf8());
    }
}

void
TermReader::commandServerRemoveAttribute(const char *body, uint32_t length)
{
    Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
    while (unm.remainingLength()) {
        std::string i = unm.parseUtf8();

        if (i.empty())
            break;
        else if (!osRestrictedServerAttribute(i))
            g_listener->commandRemoveAttribute(i);
    }
}

void
TermReader::commandServerCreateTerm(const char *body, uint32_t length)
{
    Tsq::ProtocolUnmarshaler unm(body + 16, length - 16);
    Tsq::Uuid clientId = unm.parseUuid();
    Tsq::Uuid termId = unm.parseUuid();

    Size size;
    size.setWidth(unm.parseNumber());
    size.setHeight(unm.parseNumber());

    StringMap attributes = parseTermMap(unm);
    g_listener->getOwnerAttributes(clientId, attributes);

    TermInstance *term = new TermInstance(termId, clientId, size, attributes);
    g_listener->sendWork(ListenerAddTerm, term);
}

void
TermReader::commandServerTaskPause(const char *body, uint32_t length)
{
    if (length < 64) {
        LOGNOT("Reader %p: undersize message %x\n", this, TSQ_TASK_PAUSE);
        throw ProtocolException();
    }

    Tsq::Uuid taskId(body + 32);
    Tsq::Uuid hopId(body + 48);

    g_listener->throttleTask(taskId, hopId);
}

void
TermReader::commandServerTaskInput(const char *body, uint32_t length)
{
    if (length < 48) {
        LOGNOT("Reader %p: undersize message %x\n", this, TSQ_TASK_INPUT);
        throw ProtocolException();
    }

    Tsq::Uuid taskId(body + 32);
    std::string data(body + 48, length - 48);

    g_listener->inputTask(taskId, data);
}

void
TermReader::commandServerTaskAnswer(const char *body, uint32_t length)
{
    if (length < 52) {
        LOGNOT("Reader %p: undersize message %x\n", this, TSQ_TASK_ANSWER);
        throw ProtocolException();
    }

    Tsq::Uuid taskId(body + 32);

    Tsq::ProtocolUnmarshaler unm(body + 48, length - 48);
    g_listener->answerTask(taskId, unm.parseNumber());
}

void
TermReader::commandServerCancelTask(const char *body, uint32_t length)
{
    if (length < 48) {
        LOGNOT("Reader %p: undersize message %x\n", this, TSQ_CANCEL_TASK);
        throw ProtocolException();
    }

    Tsq::Uuid taskId(body + 32);

    g_listener->cancelTask(taskId);
}

void
TermReader::commandServerFileTask(uint32_t command, const char *body, uint32_t length)
{
    Tsq::ProtocolUnmarshaler unm(body + 16, length - 16);
    TaskBase *task;

    switch (command) {
    case TSQ_UPLOAD_FILE:
        task = new FileUpload(&unm);
        break;
    default:
        task = new FileDownload(&unm);
        break;
    case TSQ_DELETE_FILE:
        task = new FileMisc(&unm, false);
        break;
    case TSQ_RENAME_FILE:
        task = new FileMisc(&unm, true);
        break;
    case TSQ_UPLOAD_PIPE:
        task = new PipeUpload(&unm);
        break;
    case TSQ_DOWNLOAD_PIPE:
        task = new PipeDownload(&unm);
        break;
    case TSQ_CONNECTING_PORTFWD:
        task = new PortOut(&unm);
        break;
    case TSQ_LISTENING_PORTFWD:
        task = new PortIn(&unm);
        break;
    case TSQ_RUN_COMMAND:
        task = new RunCommand(&unm);
        break;
    case TSQ_RUN_CONNECT:
        task = new RunConnect(&unm);
        break;
    case TSQ_MOUNT_FILE_READWRITE:
        task = new FileMount(&unm, false);
        break;
    case TSQ_MOUNT_FILE_READONLY:
        task = new FileMount(&unm, true);
        break;
    }

    g_listener->sendWork(ListenerAddTask, task);
}

void
TermReader::commandServerMonitorInput(const char *body, uint32_t length)
{
    g_monitor->sendWork(MonitorInput, new std::string(body + 32, length - 32));
}

void
TermReader::commandServerClientAttribute(const char *body, uint32_t length)
{
    Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
    Tsq::Uuid clientId = unm.parseUuid();
    Tsq::ProtocolMarshaler m;

    while (unm.remainingLength()) {
        std::string i = unm.parseUtf8();

        if (i.empty())
            break;

        m.begin(TSQ_GET_CLIENT_ATTRIBUTE_RESPONSE);
        m.addUuidPairReversed(body);
        m.addUuid(clientId);
        if (g_listener->getClientAttribute(clientId, i)) {
            m.addBytes(i);
            m_writer->submitResponse(std::move(m.result()));
        }
    }
}

void
TermReader::handleServerCommand(uint32_t command, uint32_t length, const char *body)
{
    if (length < 32) {
        LOGNOT("Reader %p: undersize message %x\n", this, command);
        throw ProtocolException();
    }

    Tsq::Uuid serverId(body);

    if (serverId == g_listener->id()) {
        switch (command) {
        case TSQ_GET_SERVER_TIME:
            commandServerTimeRequest(body);
            break;
        case TSQ_GET_SERVER_ATTRIBUTES:
            commandServerGetAttributes(body);
            break;
        case TSQ_GET_SERVER_ATTRIBUTE:
            commandServerGetAttribute(body, length);
            break;
        case TSQ_SET_SERVER_ATTRIBUTE:
            commandServerSetAttribute(body, length);
            break;
        case TSQ_REMOVE_SERVER_ATTRIBUTE:
            commandServerRemoveAttribute(body, length);
            break;
        case TSQ_CREATE_TERM:
            commandServerCreateTerm(body, length);
            break;
        case TSQ_TASK_PAUSE:
            commandServerTaskPause(body, length);
            break;
        case TSQ_TASK_INPUT:
            commandServerTaskInput(body, length);
            break;
        case TSQ_TASK_ANSWER:
            commandServerTaskAnswer(body, length);
            break;
        case TSQ_CANCEL_TASK:
            commandServerCancelTask(body, length);
            break;
        case TSQ_UPLOAD_FILE:
        case TSQ_DOWNLOAD_FILE:
        case TSQ_DELETE_FILE:
        case TSQ_RENAME_FILE:
        case TSQ_UPLOAD_PIPE:
        case TSQ_DOWNLOAD_PIPE:
        case TSQ_CONNECTING_PORTFWD:
        case TSQ_LISTENING_PORTFWD:
        case TSQ_RUN_COMMAND:
        case TSQ_RUN_CONNECT:
        case TSQ_MOUNT_FILE_READWRITE:
        case TSQ_MOUNT_FILE_READONLY:
            commandServerFileTask(command, body, length);
            break;
        case TSQ_MONITOR_INPUT:
            commandServerMonitorInput(body, length);
            break;
        case TSQ_GET_CLIENT_ATTRIBUTE:
            commandServerClientAttribute(body, length);
            break;
        default:
            LOGNOT("Reader %p: unrecognized command %x\n", this, command);
            break;
        }
    }
    else {
        Tsq::ProtocolMarshaler m(command, length, body);
        Tsq::Uuid hop;
        decltype(m_terms)::iterator i;

        switch (g_listener->forwardToServer(serverId, m.result(), hop)) {
        case 0:
            if ((i = m_terms.find(hop)) != m_terms.end())
                pushThrottlePause(body, static_cast<ConnWatch*>(i->second));
            break;
        case -1:
            LOGDBG("Reader %p: unknown recipient for command %x\n", this, command);
            break;
        }
    }
}

/*
 * Client commands
 */
void
TermReader::commandClientAnnounce(const char *body, uint32_t length)
{
    Tsq::Uuid id(body);

    if (!m_knownClients.count(id))
    {
        // Parse and rebuild announcement with incremented hop count
        Tsq::ProtocolUnmarshaler unm(body + 16, length - 16);
        Tsq::ProtocolMarshaler m(TSQ_ANNOUNCE_CLIENT, 16, body);
        unsigned version = unm.parseNumber();
        unsigned hops = unm.parseNumber();
        unsigned flags = unm.parseNumber();
        m.addNumber(version);
        m.addNumberPair(hops + 1, flags);
        m.addBytes(unm.remainingBytes(), unm.remainingLength());

        TermListener::ClientInfo info;
        info.writer = m_writer;
        info.reader = this;
        info.announce = m.result();
        info.hops = hops;
        info.flags = flags;
        parseUtf8Map(unm, info.attributes);

        // Register client
        m_knownClients.insert(id);
        if (!g_listener->registerClient(id, info)) {
            m_ignoredClients.insert(id);
            return;
        }

        // Send all current terminal data to client
        for (const auto &i: m_terms)
            if (i.second->isTermWatch)
                static_cast<TermWatch*>(i.second)->pullEverything();

        // Start servicing watches if not already
        m_writer->unblock();

        // Forward client announcement on to servers
        g_listener->forwardToServers(m.result());
    }
}

void
TermReader::commandClientRemove(const char *body, uint32_t length)
{
    Tsq::Uuid id(body);
    m_knownClients.erase(id);

    if (!m_ignoredClients.erase(id)) {
        Tsq::ProtocolMarshaler m(TSQ_REMOVE_CLIENT, length, body);
        g_listener->unregisterClient(id, m.result());
    }
}

void
TermReader::handleClientCommand(uint32_t command, uint32_t length, const char *body)
{
    if (length < 16) {
        LOGNOT("Reader %p: undersize message %x\n", this, command);
        throw ProtocolException();
    }

    switch (command) {
    case TSQ_ANNOUNCE_CLIENT:
        commandClientAnnounce(body, length);
        break;
    case TSQ_REMOVE_CLIENT:
        commandClientRemove(body, length);
        break;
    default:
        LOGNOT("Reader %p: unrecognized command %x\n", this, command);
        break;
    }
}

/*
 * Term commands
 */
void
TermReader::commandTermGetAttributes(ConnWatch *watch, const char *body)
{
    Tsq::ProtocolMarshaler m(watch->isTermWatch ?
                             TSQ_GET_TERM_ATTRIBUTES_RESPONSE :
                             TSQ_GET_CONN_ATTRIBUTES_RESPONSE);
    m.addUuidPairReversed(body);
    m.addBytes(watch->parent()->commandGetAttributes());
    m_writer->submitResponse(std::move(m.result()));
}

void
TermReader::commandTermGetAttribute(ConnWatch *watch, const char *body, uint32_t length)
{
    Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
    Tsq::ProtocolMarshaler m;
    uint32_t command = watch->isTermWatch ?
        TSQ_GET_TERM_ATTRIBUTE_RESPONSE :
        TSQ_GET_CONN_ATTRIBUTE_RESPONSE;

    while (unm.remainingLength()) {
        std::string i = unm.parseUtf8();

        if (i.empty())
            break;

        m.begin(command);
        m.addUuidPairReversed(body);
        m.addBytes(watch->parent()->commandGetAttribute(i));
        m_writer->submitResponse(std::move(m.result()));
    }
}

void
TermReader::commandTermSetAttribute(ConnWatch *watch, const char *body, uint32_t length)
{
    Tsq::Uuid client(body + 16);
    Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);

    while (unm.remainingLength()) {
        std::string key = unm.parseUtf8();

        if (key.empty())
            break;
        else if (osRestrictedTermAttribute(key, watch->parent()->testOwner(client)))
            unm.parseUtf8();
        else
            watch->parent()->commandSetAttribute(key, unm.parseUtf8());
    }
}

void
TermReader::commandTermRemoveAttribute(ConnWatch *watch, const char *body, uint32_t length)
{
    Tsq::Uuid client(body + 16);
    Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);

    while (unm.remainingLength()) {
        std::string i = unm.parseUtf8();

        if (i.empty())
            break;
        else if (!osRestrictedTermAttribute(i, watch->parent()->testOwner(client)))
            watch->parent()->commandRemoveAttribute(i);
    }
}

void
TermReader::commandTermInput(ConnWatch *watch, const char *body, uint32_t length)
{
    Tsq::Uuid client(body + 16);

    if (watch->parent()->testSender(client)) {
        TermWatch *w = static_cast<TermWatch*>(watch);
        if (!w->emulator()->termSend(body + 32, length - 32))
            pushThrottlePause(body, watch);
    }
}

void
TermReader::commandTermMouse(ConnWatch *watch, const char *body, uint32_t length)
{
    Tsq::Uuid client(body + 16);

    if (watch->parent()->testOwner(client)) {
        Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
        unsigned event = unm.parseNumber();
        unsigned x = unm.parseNumber();
        unsigned y = unm.parseNumber();

        TermWatch *w = static_cast<TermWatch*>(watch);
        if (!w->emulator()->termMouse(event, x, y))
            pushThrottlePause(body, watch);
    }
}

void
TermReader::commandTermGetRows(ConnWatch *watch, const char *body, uint32_t length)
{
    if (!watch->isTermWatch)
        return;

    Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
    index_t start = unm.parseNumber64();
    index_t end = unm.parseNumber64();
    unsigned bufid = unm.parseNumber() & 0xff;
    std::vector<CellRow> rows;
    std::vector<Region> regions;

    static_cast<TermWatch*>(watch)->term()->commandGetRows(
        bufid, start, end, rows, regions);

    Tsq::ProtocolMarshaler m;
    m.begin(TSQ_BEGIN_OUTPUT_RESPONSE);
    m.addUuidPairReversed(body);
    m_writer->submitResponse(std::move(m.result()));

    for (const auto &region: regions)
    {
        m.begin(TSQ_REGION_UPDATE_RESPONSE);
        m.addUuidPairReversed(body);
        m.addNumberPair(region.id, region.wireType());
        m.addNumberPair(region.flags, region.parent);
        m.addNumber64(region.startRow);
        m.addNumber64(region.endRow);
        m.addNumberPair(region.startCol, region.endCol);
        for (const auto &i: region.attributes)
            m.addStringPair(i.first, i.second);
        m_writer->submitResponse(std::move(m.result()));
    }

    for (auto &&row: rows)
    {
        m.begin(TSQ_ROW_CONTENT_RESPONSE);
        m.addUuidPairReversed(body);
        m.addNumber64(start++);
        m.addNumberPair(row.flags|bufid, row.modtime);
        m.addNumber(row.numRanges());

        #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        for (unsigned z = 0; z < row.m_ranges.size(); ++z)
            row.m_ranges[z] = __builtin_bswap32(row.m_ranges[z]);
        #endif

        m.addBytes((const char *)row.m_ranges.data(), row.m_ranges.size() * 4);
        m.addBytes(row.m_str.data(), row.m_str.size());
        m_writer->submitResponse(std::move(m.result()));
    }

    m.begin(TSQ_END_OUTPUT_RESPONSE);
    m.addUuidPairReversed(body);
    m_writer->submitResponse(std::move(m.result()));
}

void
TermReader::commandTermGetImage(ConnWatch *watch, const char *body, uint32_t length)
{
    if (watch->isTermWatch) {
        Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
        contentid_t id = unm.parseNumber64();

        Tsq::ProtocolMarshaler m;
        m.begin(TSQ_IMAGE_CONTENT_RESPONSE);
        m.addUuidPairReversed(body);
        m.addNumber64(id);

        if (static_cast<TermWatch*>(watch)->term()->commandGetContent(id, &m))
            m_writer->submitResponse(std::move(m.result()));
    }
}

void
TermReader::commandTermDownloadImage(ConnWatch *watch, const char *body, uint32_t length)
{
    if (watch->isTermWatch) {
        Tsq::ProtocolUnmarshaler unm(body + 16, length - 16);
        TaskBase *task = new ImageDownload(&unm, static_cast<TermWatch*>(watch)->term());
        g_listener->sendWork(ListenerAddTask, task);
    }
}

void
TermReader::commandTermResizeBuffer(ConnWatch *watch, const char *body, uint32_t length)
{
    Tsq::Uuid client(body + 16);

    if (watch->parent()->testOwner(client)) {
        Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
        unsigned spec = unm.parseNumber();
        watch->parent()->sendWork(TermResizeBuffer, spec & 0xff, spec >> 8 & 0xff);
    }
}

void
TermReader::commandTermResizeTerm(ConnWatch *watch, const char *body, uint32_t length)
{
    Tsq::Uuid client(body + 16);

    if (watch->parent()->testOwner(client)) {
        Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
        int width = unm.parseNumber();
        int height = unm.parseNumber();
        watch->parent()->sendWork(TermResizeTerm, width, height);
    }
}

void
TermReader::commandTermDuplicate(ConnWatch *watch, const char *body, uint32_t length)
{
    if (watch->isTermWatch) {
        Tsq::ProtocolUnmarshaler unm(body + 16, length - 16);
        Tsq::Uuid clientId = unm.parseUuid();
        Tsq::Uuid termId = unm.parseUuid();

        Size size;
        size.setWidth(unm.parseNumber());
        size.setHeight(unm.parseNumber());

        StringMap attributes = parseTermMap(unm);
        g_listener->getOwnerAttributes(clientId, attributes);

        auto *term = static_cast<TermWatch*>(watch)->term()->commandDuplicate(
            termId, clientId, size, attributes);
        g_listener->sendWork(ListenerAddTerm, term);
    }
}

void
TermReader::commandTermReset(ConnWatch *watch, const char *body, uint32_t length)
{
    Tsq::Uuid client(body + 16);

    if (watch->parent()->testOwner(client)) {
        Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
        watch->parent()->sendWork(TermReset, unm.parseNumber());
    }
}

void
TermReader::commandTermSendSignal(ConnWatch *watch, const char *body, uint32_t length)
{
    Tsq::Uuid client(body + 16);

    if (watch->parent()->testOwner(client)) {
        Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
        watch->parent()->sendWork(TermSignal, unm.parseNumber());
    }
}

void
TermReader::commandTermChangeOwner(ConnWatch *watch, const char *body)
{
    Tsq::Uuid clientId(body + 16);
    watch->parent()->setOwner(clientId);
}

void
TermReader::commandRegionCreate(ConnWatch *watch, const char *body, uint32_t length)
{
    if (watch->isTermWatch) {
        Tsq::ProtocolUnmarshaler unm(body + 16, length - 16);
        Tsq::Uuid clientId = unm.parseUuid();
        uint8_t bufid = unm.parseNumber();

        Region *region = new Region((Tsq::RegionType)unm.parseNumber());
        if (region->type == Tsq::RegionInvalid)
            region->type = Tsq::RegionUser;

        region->id = INVALID_REGION_ID;
        region->bufid = bufid;
        region->startRow = unm.parseNumber64();
        region->endRow = unm.parseNumber64();
        region->startCol = unm.parseNumber();
        region->endCol = unm.parseNumber();

        while (unm.remainingLength()) {
            std::string key = unm.parseUtf8();

            if (key.empty())
                break;

            region->attributes[key] = unm.parseUtf8();
        }

        // Set some additional attributes
        StringMap attributes;
        g_listener->getOwnerAttributes(clientId, attributes);
        region->attributes[Tsq::attr_REGION_USER] = attributes[Tsq::attr_OWNER_USER];
        region->attributes[Tsq::attr_REGION_HOST] = attributes[Tsq::attr_OWNER_HOST];
        region->attributes[Tsq::attr_REGION_STARTED] = std::to_string(osWalltime());

        static_cast<TermWatch*>(watch)->term()->commandCreateRegion(region);
    }
}

void
TermReader::commandRegionGet(ConnWatch *watch, const char *body, uint32_t length)
{
    if (watch->isTermWatch) {
        Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
        uint8_t bufid = unm.parseNumber();
        regionid_t regid = unm.parseNumber();
        Region region(Tsq::RegionLowerBound);

        bool rc = static_cast<TermWatch*>(watch)->term()->commandGetRegion(
            bufid, regid, region);

        if (rc) {
            Tsq::ProtocolMarshaler m(TSQ_BEGIN_OUTPUT_RESPONSE);
            m.addUuidPairReversed(body);
            m_writer->submitResponse(std::move(m.result()));

            m.begin(TSQ_REGION_UPDATE_RESPONSE);
            m.addUuidPairReversed(body);
            m.addNumberPair(region.id, region.wireType());
            m.addNumberPair(region.flags, region.parent);
            m.addNumber64(region.startRow);
            m.addNumber64(region.endRow);
            m.addNumberPair(region.startCol, region.endCol);
            for (const auto &i: region.attributes)
                m.addStringPair(i.first, i.second);
            m_writer->submitResponse(std::move(m.result()));

            m.begin(TSQ_END_OUTPUT_RESPONSE);
            m.addUuidPairReversed(body);
            m_writer->submitResponse(std::move(m.result()));
        }
    }
}

void
TermReader::commandRegionRemove(ConnWatch *watch, const char *body, uint32_t length)
{
    if (watch->isTermWatch) {
        Tsq::ProtocolUnmarshaler unm(body + 32, length - 32);
        uint8_t bufid = unm.parseNumber();
        regionid_t regid = unm.parseNumber();

        watch->parent()->sendWork(TermRemoveRegion, regid, bufid);
    }
}

void
TermReader::handleTermCommand(uint32_t command, uint32_t length, const char *body)
{
    if (length < 32) {
        LOGNOT("Reader %p: undersize message %x\n", this, command);
        throw ProtocolException();
    }

    Tsq::Uuid termId(body);

    auto i = m_terms.find(termId);
    if (i != m_terms.end()) {
        ConnWatch *watch = dynamic_cast<ConnWatch*>(i->second);
        if (!watch) {
            LOGDBG("Reader %p: not a terminal during command %x\n", this, command);
            return;
        }

        switch (command) {
        case TSQ_INPUT:
            commandTermInput(watch, body, length);
            break;
        case TSQ_MOUSE_INPUT:
            commandTermMouse(watch, body, length);
            break;
        case TSQ_ROW_CONTENT:
            commandTermGetRows(watch, body, length);
            break;
        case TSQ_BUFFER_CAPACITY:
            commandTermResizeBuffer(watch, body, length);
            break;
        case TSQ_IMAGE_CONTENT:
            commandTermGetImage(watch, body, length);
            break;
        case TSQ_DOWNLOAD_IMAGE:
            commandTermDownloadImage(watch, body, length);
            break;
        case TSQ_GET_TERM_ATTRIBUTES:
            commandTermGetAttributes(watch, body);
            break;
        case TSQ_GET_TERM_ATTRIBUTE:
            commandTermGetAttribute(watch, body, length);
            break;
        case TSQ_SET_TERM_ATTRIBUTE:
            commandTermSetAttribute(watch, body, length);
            break;
        case TSQ_REMOVE_TERM_ATTRIBUTE:
            commandTermRemoveAttribute(watch, body, length);
            break;
        case TSQ_RESIZE_TERM:
            commandTermResizeTerm(watch, body, length);
            break;
        case TSQ_REMOVE_TERM:
            watch->parent()->sendWork(TermClose, TSQ_STATUS_CLOSED);
            break;
        case TSQ_DUPLICATE_TERM:
            commandTermDuplicate(watch, body, length);
            break;
        case TSQ_RESET_TERM:
            commandTermReset(watch, body, length);
            break;
        case TSQ_CHANGE_OWNER:
            commandTermChangeOwner(watch, body);
            break;
        case TSQ_REQUEST_DISCONNECT:
            watch->parent()->sendWork(TermDisconnect, TSQ_STATUS_NORMAL);
            break;
        case TSQ_TOGGLE_SOFT_SCROLL_LOCK:
            watch->parent()->sendWork(TermSetScrollLock, 0);
            break;
        case TSQ_SEND_SIGNAL:
            commandTermSendSignal(watch, body, length);
            break;
        case TSQ_CREATE_REGION:
            commandRegionCreate(watch, body, length);
            break;
        case TSQ_GET_REGION:
            commandRegionGet(watch, body, length);
            break;
        case TSQ_REMOVE_REGION:
            commandRegionRemove(watch, body, length);
            break;
        default:
            LOGNOT("Reader %p: unrecognized command %x\n", this, command);
            break;
        }
    }
    else {
        Tsq::ProtocolMarshaler m(command, length, body);
        Tsq::Uuid hop;
        decltype(m_terms)::iterator i;

        switch (g_listener->forwardToTerm(termId, m.result(), hop)) {
        case 0:
            if ((i = m_terms.find(hop)) != m_terms.end())
                pushThrottlePause(body, static_cast<ConnWatch*>(i->second));
            break;
        case -1:
            LOGDBG("Reader %p: unknown recipient for command %x\n", this, command);
            break;
        }
    }
}

bool
TermReader::protocolCallback(uint32_t command, uint32_t length, const char *body)
{
//    LOGDBG("Reader %p: command %x, length %u\n", this, command, length);

    switch (command & TSQ_CMDTYPE_MASK) {
    case TSQ_CMDTYPE_PLAIN:
        return handlePlainCommand(command, length, body);
    case TSQ_CMDTYPE_SERVER:
        handleServerCommand(command, length, body);
        return true;
    case TSQ_CMDTYPE_TERM:
        handleTermCommand(command, length, body);
        return true;
    case TSQ_CMDTYPE_CLIENT:
        handleClientCommand(command, length, body);
        return true;
    default:
        LOGNOT("Reader %p: unrecognized command %x\n", this, command);
        return true;
    }
}
