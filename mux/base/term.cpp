// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "term.h"
#include "output.h"
#include "status.h"
#include "filemon.h"
#include "termwatch.h"
#include "writer.h"
#include "listener.h"
#include "zombies.h"
#include "exception.h"
#include "xterm/xterm.h"
#include "app/args.h"
#include "os/attr.h"
#include "os/dir.h"
#include "os/pty.h"
#include "os/time.h"
#include "os/process.h"
#include "os/logging.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/handshake.h"
#include "lib/term.h"
#include "lib/enums.h"
#include "lib/attr.h"
#include "lib/attrstr.h"
#include "config.h"

#include <cstdio>
#include <climits>
#include <unistd.h>
#include <csignal>

#define TR_HALTMSG1 TL("server", "Restarting process", "halt1")
#define TR_HALTMSG2 TL("server", "Emulator execution has finished", "halt2")
#define TR_HALTMSG3 TL("server", "%1ms required before autoclose (actual: %2ms)", "halt3")

void
TermInstance::setup(const Tsq::Uuid &id, const Tsq::Uuid &owner, Size size,
                    OwnershipInfo *oi, EmulatorParams *params)
{
    m_id = id;
    m_owner = owner;
    m_machine = new Tsq::TermClientProtocol(this);
    m_attributes = std::move(oi->attributes);

    m_params = new PtyParams{};
    m_params->daemon = true;
    m_params->exitDelay = true;

    configuredInitParams(params);
    m_filemon = new TermFilemon(params->fileLimit, this);
    m_translator = params->translator;
    m_status = new TermStatusTracker(m_translator, std::move(oi->environ));

    m_attributes[Tsq::attr_ID] = m_id.str();
    m_attributes[Tsq::attr_STARTED] = std::to_string(osBasetime(&m_baseTime));
    m_attributes[Tsq::attr_SESSION_COLS] = std::to_string(size.width());
    m_attributes[Tsq::attr_SESSION_ROWS] = std::to_string(size.height());
    m_attributes[Tsq::attr_ENCODING] = params->unicoding->spec().name();
}

TermInstance::TermInstance(const Tsq::Uuid &id, const Tsq::Uuid &owner,
                           Size size, OwnershipInfo *oi) :
    ConnInstance("term", true)
{
    EmulatorParams params;
    setup(id, owner, size, oi, &params);

    m_emulator = new XTermEmulator(this, size, params);
}

TermInstance::TermInstance(const Tsq::Uuid &id, const Tsq::Uuid &owner,
                           Size size, OwnershipInfo *oi,
                           const TermInstance *copyfrom) :
    ConnInstance("term", true)
{
    EmulatorParams params;
    setup(id, owner, size, oi, &params);

    m_emulator = copyfrom->m_emulator->duplicate(this, size);
}

TermInstance::~TermInstance()
{
    delete m_emulator;
    delete m_status;
    delete m_filemon;
    delete m_params;

    for (auto i: m_incomingRegions)
        delete i;
}

/*
 * Other threads
 */
TermInstance *
TermInstance::commandDuplicate(const Tsq::Uuid &id, const Tsq::Uuid &owner,
                               Size size, OwnershipInfo *oi)
{
    StateLock slock(this, false);

    for (const auto &i: m_attributes)
        if (!osRestrictedTermAttribute(i.first, false))
            oi->attributes.emplace(i);

    return new TermInstance(id, owner, size, oi, this);
}

void
TermInstance::commandGetRows(uint8_t bufid, index_t start, index_t end,
                             std::vector<CellRow> &rows,
                             std::vector<Region> &regions)
{
    TermBuffer *buffer = m_emulator->buffer(!!bufid);
    ++end;

    StateLock slock(this, false);

    buffer->pullRegions(start, end, regions);

    while (start < buffer->size() && start < end)
        rows.emplace_back(buffer->constRow(start++));
}

bool
TermInstance::commandGetContent(contentid_t id, Tsq::ProtocolMarshaler *m)
{
    StateLock lock(this, false);

    const std::string *data = m_emulator->getContentPtr(id);
    if (data && data->size() < IMAGE_SIZE_THRESHOLD) {
        m->addBytes(*data);
        return true;
    }

    return false;
}

bool
TermInstance::commandGetRegion(uint8_t bufid, regionid_t id, Region &region)
{
    StateLock lock(this, false);
    const Region *ptr = m_emulator->buffer(!!bufid)->safeRegion(id);
    if (ptr) {
        region = *ptr;
        return true;
    }
    return false;
}

void
TermInstance::commandCreateRegion(Region *region)
{
    FlexLock lock(this);
    m_incomingRegions.insert(region);
    sendWorkAndUnlock(lock, TermCreateRegion, region);
}

bool
TermInstance::reportHardScrollLock(bool enabled, bool query)
{
    if (query && (m_fd == -1 || !osGetTerminalLockable(m_fd)))
        return false;

    sendWork(TermSetScrollLock, 1, enabled);
    return true;
}

void
TermInstance::reportDirectoryUpdate(const std::string &msg)
{
    Lock lock(this);

    for (auto w: m_watches) {
        // two locks held
        static_cast<TermWatch*>(w)->pushDirectoryUpdate(msg);
    }
}

void
TermInstance::reportFileUpdate(const std::string &name, const std::string &msg)
{
    Lock lock(this);

    for (auto w: m_watches) {
        // two locks held
        static_cast<TermWatch*>(w)->pushFileUpdate(name, msg);
    }
}

void
TermInstance::reportFileUpdates(const StringMap &map)
{
    Lock lock(this);

    for (auto w: m_watches) {
        // two locks held
        static_cast<TermWatch*>(w)->pushFileUpdates(map);
    }
}

/*
 * This thread
 */
void
TermInstance::handleStatusAttributes(StringMap &map)
{
    {
        StateLock slock(this, true);

        for (auto &&i: map) {
            m_attributes[i.first] = i.second;

            if (i.first == Tsq::attr_PROC_CWD)
                // two locks held
                m_filemon->monitor(i.second);

            std::string spec = i.first;
            spec.push_back('\0');
            spec.append(i.second);
            spec.push_back('\0');

            i.second = std::move(spec);
        }
    }

    Lock lock(this);

    for (auto watch: m_watches) {
        // two locks held
        watch->pushAttributeChanges(map);
    }
}

void
TermInstance::launch()
{
    int64_t now = osMonotime();
    char devpath[PATH_MAX];

    std::string msg = configuredStartParams(m_params);
    m_params->width = (unsigned short)m_emulator->size().width();
    m_params->height = (unsigned short)m_emulator->size().height();
    m_params->sleepTime = (now - m_launchTime < 1000) ? 1 : 0;

    m_launchTime = now;
    m_timeout = 0;
    m_status->start(m_params);

    if (!msg.empty())
        handleTermEvent(const_cast<char*>(msg.data()), msg.size(), false);

    try {
        setfd(osForkTerminal(*m_params, &m_pid, devpath));
        m_haveOutcome = false;
        m_haveClosed = false;
    }
    catch (const std::exception &e) {
        const char *msg = e.what();
        LOGWRN("Term %p: failed to start process: %s\n", this, msg);
        handleTermEvent(const_cast<char*>(msg), strlen(msg), false);
    }

    if (m_pid) {
        g_reaper->registerProcess(this, m_pid);

        m_status->changedMap().emplace(Tsq::attr_PROC_DEV, devpath);

        if (!m_output->started())
            m_output->start(-1);
    }

    handleStatusAttributes(m_status->changedMap());
}

void
TermInstance::pushChanges(bool activate)
{
    Lock lock(this);

    for (auto w: m_watches) {
        TermWatch *watch = static_cast<TermWatch*>(w);
        BaseWatch::Lock wlock(w);

        // two locks held
        watch->state.flagsChanged |= m_emulator->flagsChanged;

        watch->state.bufferChanged[0][0] |= m_emulator->bufferChanged[0][0];
        watch->state.bufferChanged[0][1] |= m_emulator->bufferChanged[0][1];
        watch->state.bufferChanged[1][0] |= m_emulator->bufferChanged[1][0];
        watch->state.bufferChanged[1][1] |= m_emulator->bufferChanged[1][1];
        watch->state.bufferSwitched |= m_emulator->bufferSwitched;

        watch->state.sizeChanged |= m_emulator->sizeChanged;
        watch->state.cursorChanged |= m_emulator->cursorChanged;
        watch->state.bellCount += m_emulator->bellCount;

        watch->state.pushContent();

        for (const auto &i: m_emulator->changedAttributes)
            watch->attributes[i.first] = i.second;

        if (activate)
            w->activate();
    }
}

void
TermInstance::handleTermEvent(char *buf, unsigned len, bool running)
{
    bool activate = true;
    bool chflags = false;

    // update modtime
    m_modTime = osModtime(m_baseTime);

    // determine ratelimit status
    switch (m_rateStatus) {
    case 2:
        if (m_modTime - m_ratePushTime >= RATELIMIT_INTERVAL) {
            m_ratePushTime = m_modTime;
        } else {
            activate = false;
        }
        break;
    case 1:
        if (m_modTime - m_rateIdleTime > RATELIMIT_THRESHOLD && !m_haveConnection) {
            m_rateStatus = 2;
            m_ratePushTime = m_modTime;
            chflags = m_emulator->setFlag(Tsq::RateLimited, true);
        }
        break;
    default:
        m_rateStatus = 1;
        m_rateIdleTime = m_modTime;
        break;
    }

    // step emulator
    if (m_emulator->termEvent(buf, len, running, chflags) ||
        !m_emulator->buffer(0)->changedRows().empty() ||
        !m_emulator->buffer(0)->changedRegions().empty() ||
        !m_emulator->buffer(1)->changedRows().empty() ||
        !m_emulator->buffer(1)->changedRegions().empty() ||
        !m_emulator->changedAttributes.empty())
        // report changes to watches
        pushChanges(activate);
}

void
TermInstance::handleTermReset(const char *buf, unsigned len, Tsq::ResetFlags arg)
{
    // set idle
    m_rateStatus = 0;

    // reset emulator
    if (m_emulator->termReset(buf, len, arg) ||
        !m_emulator->buffer(0)->changedRows().empty() ||
        !m_emulator->buffer(0)->changedRegions().empty() ||
        !m_emulator->buffer(1)->changedRows().empty() ||
        !m_emulator->buffer(1)->changedRegions().empty() ||
        !m_emulator->changedAttributes.empty())
        // report changes to watches
        pushChanges();
}

void
TermInstance::handleTermResize(Size size)
{
    // resize and then step emulator
    if (m_emulator->termResize(size))
        // report changes to watches
        pushChanges();
}

void
TermInstance::handleMouseMove(Point mousePos)
{
    if (m_emulator->moveMouse(mousePos))
    {
        Lock lock(this);

        for (auto w: m_watches) {
            BaseWatch::ActivatorLock wlock(w);
            static_cast<TermWatch*>(w)->state.mouseMoved = true;
        }
    }
}

void
TermInstance::handleBufferResize(uint8_t bufid, uint8_t caporder)
{
    if (m_emulator->bufferResize(bufid, caporder))
        pushChanges();
}

void
TermInstance::handleUpdateEnviron(SharedStringMap *environ)
{
    if (m_status->setEnviron(*environ))
        handleStatusAttributes(m_status->changedMap());

    delete environ;
}

void
TermInstance::handleCreateRegion(Region *region)
{
    regionid_t id = INVALID_REGION_ID;

    if (region->bufid == 0) {
        StateLock slock(this, true);
        id = m_emulator->buffer(0)->addUserRegion(region);
    }

    Lock lock(this);
    m_incomingRegions.erase(region);

    if (id != INVALID_REGION_ID) {
        for (auto watch: m_watches) {
            // two locks held
            static_cast<TermWatch*>(watch)->pushUserRegion(id);
        }
    } else {
        delete region;
    }
}

void
TermInstance::handleRemoveRegion(regionid_t id, uint8_t bufid)
{
    bool existed = false;

    if (bufid == 0) {
        StateLock slock(this, true);
        existed = m_emulator->buffer(0)->removeUserRegion(id);
    }

    if (existed) {
        Lock lock(this);

        for (auto watch: m_watches) {
            // two locks held
            static_cast<TermWatch*>(watch)->pushUserRegion(id);
        }
    }
}

void
TermInstance::handleScrollLock(bool hard, bool enabled)
{
    if (hard) {
        if (m_emulator->setFlag(Tsq::HardScrollLock, enabled))
            pushChanges();
    } else {
        togglefd();
        m_emulator->setFlag(Tsq::SoftScrollLock, !m_fds[1].events);
        pushChanges();
    }
}

bool
TermInstance::handleFd()
{
    char buf[TERM_BUFSIZE];
    ssize_t rc;

    m_timeout = 0;

    // Note: must leave 8 bytes open for insertion of running UTF-8
    rc = read(m_fd, buf + 8, sizeof(buf) - 8);
    if (rc <= 0) {
        if (rc < 0 && (errno == EINTR || errno == EAGAIN))
            return true;

        goto halt;
    }

    // LOGDBG("Term %p: Read %d bytes from pts\n", this, rc);
    handleTermEvent(buf + 8, rc, true);
    return true;
halt:
    disconnect(DisReport, 0);
    closefd();

    m_haveClosed = true;
    if (m_haveOutcome)
        halt();
    return true;
}

void
TermInstance::resetRatelimit()
{
    switch (m_rateStatus) {
    case 2:
        m_emulator->setFlag(Tsq::RateLimited, false);
        pushChanges();
        // fallthru
    case 1:
        m_rateStatus = 0;
    }
}

bool
TermInstance::handleIdle()
{
    // LOGDBG("Term %p: idle at %d\n", this, m_timeout);
    if (m_status->update(m_fd, m_pid))
        handleStatusAttributes(m_status->changedMap());

    switch (m_timeout) {
    case 0:
        m_timeout = IDLE_INITIAL_TIMEOUT;
        break;
    case IDLE_LAST_TIMEOUT:
        m_timeout = -1;
        break;
    case IDLE_INITIAL_TIMEOUT:
        resetRatelimit();
        // fallthru
    default:
        m_timeout *= 2;
        break;
    }

    return true;
}

void
TermInstance::halt()
{
    char buf[512];
    const char *msg = m_status->outcomeStr();
    const char *prefix = "\xc2\x9d""133;D\x07";
    Tsq::ResetFlags flags = Tsq::ResetEmulator;

    switch (configuredExitAction()) {
    case Tsq::ExitActionClear:
        flags |= Tsq::ClearScrollback|Tsq::ClearScreen;
        // fallthru
    case Tsq::ExitActionRestart:
        snprintf(buf, sizeof(buf), "%s%s - %s\r\n", prefix, msg, TR_HALTMSG1);
        handleTermReset(buf, strlen(buf), flags);

        m_output->reset();
        launch();

        break;
    default:
        snprintf(buf, sizeof(buf), "%s%s\r\n%s\r\n", prefix, msg, TR_HALTMSG2);
        handleTermEvent(buf, strlen(buf), false);

        auto acparm = configuredAutoClose();
        int64_t runtime = osMonotime() - m_launchTime;

        if (runtime < acparm.first) {
            std::string str;
            g_args->arg(str, TR_HALTMSG3, acparm.first, runtime);
            str.append("\r\n", 2);
            handleTermEvent(const_cast<char*>(str.data()), str.size(), false);
        }
        else if (m_status->autoClose(acparm.second)) {
            sendWork(TermClose, TSQ_STATUS_NORMAL);
        }

        break;
    }
}

void
TermInstance::handleProcessExited(int disposition)
{
    m_status->setOutcome(m_pid, disposition);
    handleStatusAttributes(m_status->changedMap());

    m_pid = 0;
    m_haveOutcome = true;
    if (m_haveClosed)
        halt();
}

void
TermInstance::reportAttributeChange(const std::string &key, const std::string &value)
{
    if (key == Tsq::attr_PROFILE_NFILES)
        m_filemon->setLimit(value);
    else
        m_emulator->reportAttributeChange(key, value);
}

void
TermInstance::postConnect()
{
    m_emulator->setAttribute(Tsq::attr_PEER, m_remoteId.str());
}

void
TermInstance::postDisconnect(bool locked)
{
    if (locked)
        m_emulator->removeAttribute(Tsq::attr_PEER);
    else
        commandRemoveAttribute(Tsq::attr_PEER);
}

bool
TermInstance::handleWork(const WorkItem &item)
{
    m_timeout = 0;

    switch (item.type) {
    case TermClose:
        return handleClose(DisActive, item.value);
    case TermDisconnect:
        disconnect(DisActive|DisReport, item.value);
        break;
    case TermWatchAdded:
        // do nothing
        break;
    case TermWatchReleased:
        return handleWatchReleased((TermWatch*)item.value);
    case TermServerReleased:
        return handleServerReleased((ServerProxy*)item.value);
    case TermProxyReleased:
        handleProxyReleased((TermProxy*)item.value);
        break;
    case TermProcessExited:
        handleProcessExited(item.value);
        break;
    case TermResizeTerm:
        handleTermResize(Size(item.value, item.value2));
        break;
    case TermResizeBuffer:
        handleBufferResize(item.value, item.value2);
        break;
    case TermCreateRegion:
        handleCreateRegion((Region*)item.value);
        break;
    case TermRemoveRegion:
        handleRemoveRegion(item.value, item.value2);
        break;
    case TermUpdateEnviron:
        handleUpdateEnviron((SharedStringMap*)item.value);
        break;
    case TermReset:
        handleTermReset("", 0, item.value);
        break;
    case TermMoveMouse:
        handleMouseMove(Point(item.value, item.value2));
        break;
    case TermInputSent:
        resetRatelimit();
        break;
    case TermSetScrollLock:
        handleScrollLock(item.value, item.value2);
        break;
    case TermSignal:
        m_status->sendSignal(m_fd, item.value);
        break;
    default:
        break;
    }

    return true;
}

void
TermInstance::threadMain()
{
    m_filemon->start(-1);
    launch();

    try {
        runDescriptorLoop();
    }
    catch (const TsqException &e) {
        LOGERR("Term %p: %s\n", this, e.what());
        closefd();
    } catch (const std::exception &e) {
        LOGERR("Term %p: caught exception: %s\n", this, e.what());
        closefd();
    }

    m_filemon->stop(0);
    m_filemon->join();

    if (m_pid) {
        g_reaper->abandonProcess(m_pid);
        // osKillProcess(m_pid, SIGHUP);
        // LOGDBG("Term %p: sent SIGHUP to pid %d (closing)\n", this, m_pid);
    }

    LOGDBG("Term %p: goodbye\n", this);
    g_listener->sendWork(ListenerRemoveTerm, this);
}

/*
 * Called by emulator while locked
 */
void
TermInstance::resizeFd(Size size)
{
    if (m_fd != -1)
        osResizeTerminal(m_fd, size.width(), size.height());
}

bool
TermInstance::setAttribute(const std::string &key, const std::string &value)
{
    auto i = m_attributes.find(key);
    if (i == m_attributes.end() || i->second != value) {
        m_attributes[key] = value;
        return true;
    }
    return false;
}

bool
TermInstance::removeAttribute(const std::string &key)
{
    return m_attributes.erase(key);
}

const std::string &
TermInstance::getAttribute(const std::string &key, bool *found) const
{
    auto i = m_attributes.find(key), j = m_attributes.end();
    if (found)
        *found = (i != j);
    return i != j ? i->second : g_mtstr;
}

const char *
TermInstance::getAnswerback() const
{
    const size_t len = sizeof(TSQ_ENV_ANSWERBACK) - 1;
    const char *ptr = m_params->env.c_str(), *end = ptr + m_params->env.size();
    const char *result = "";

    for (; ptr < end; ptr += strlen(ptr) + 1)
        if (!strncmp(ptr, TSQ_ENV_ANSWERBACK, len))
            result = ptr + len;

    for (Codepoint c: Codestring(result))
        if (XTermStateMachine::isControlCode(c))
            return "";

    return result;
}

const StringMap &
TermInstance::resetEnviron() const
{
    m_status->resetEnviron();
    return m_status->changedMap();
}

std::string
TermInstance::termCommand(const Codestring &body)
{
    Tsq::ClientHandshake handshake(id().buf, true);
    unsigned protocolType = TSQ_PROTOCOL_REJECT;
    unsigned clientVersion = TSQ_STATUS_DUPLICATE_CONN;

    if (m_haveConnection) {
        disconnect(DisReport|DisLocked, 0);
        LOGWRN("Term %p: Received handshake message when connection already established\n", this);
        goto out;
    }

    try {
        const std::string &str = body.str();

        if (handshake.processHello(str.data(), str.size()) != Tsq::ShakeSuccess)
            throw TsqException("Received invalid handshake message from server");

        if (handshake.protocolVersion != TSQ_PROTOCOL_VERSION) {
            LOGERR("Term %p: Unsupported server protocol version %u\n",
                   this, handshake.protocolVersion);
            clientVersion = TSQ_STATUS_PROTOCOL_MISMATCH;
        }
        else {
            m_remoteId = Tsq::Uuid(handshake.serverId);

            if (g_listener->checkServer(m_remoteId)) {
                protocolType = TSQ_PROTOCOL_TERM;
                clientVersion = CLIENT_VERSION;
                m_haveConnection = true;

                // Place terminal in raw mode
                osMakeRawTerminal(m_fd);
            }
        }
    }
    catch (const std::exception &e) {
        LOGERR("Term %p: %s\n", this, e.what());
        clientVersion = TSQ_STATUS_SERVER_ERROR;
    }
out:
    return handshake.getResponse(clientVersion, protocolType);
}

void
TermInstance::termData(const Codestring &body)
{
    if (m_haveConnection) {
        bool ok;

        try {
            ok = m_machine->connRead(body.str().data(), body.str().size());
        }
        catch (const TsqException &e) {
            LOGERR("Term %p: %s\n", this, e.what());
            ok = false;
        }

        if (!ok)
            disconnect(DisReport|DisLocked, 0);
    }
}

/*
 * Attribute parsing
 */
void
TermInstance::configuredInitParams(EmulatorParams *params) const
{
    // Note: only call from constructor (no locking)
    params->flags = Tsq::DefaultTermFlags;
    params->caporder = TERM_DEF_CAPORDER;
    params->promptNewline = false;
    params->scrollClear = false;
    params->fileLimit = FILEMON_DEFAULT_LIMIT;
    std::string lang, unicoding = TSQ_UNICODE_DEFAULT;
    char *endptr;

    StringMap::const_iterator j = m_attributes.end(), i;

    if ((i = m_attributes.find(Tsq::attr_PROFILE_FLAGS)) != j) {
        params->flags |= strtoull(i->second.c_str(), &endptr, 10);
        if (*endptr == '\x1f')
            params->flags &= ~strtoull(endptr + 1, NULL, 10);
    }
    if ((i = m_attributes.find(Tsq::attr_PREF_PALETTE)) != j) {
        params->palette = i->second;
    }
    if ((i = m_attributes.find(Tsq::attr_PROFILE_CAPORDER)) != j) {
        params->caporder = strtoul(i->second.c_str(), NULL, 10);
    }
    if ((i = m_attributes.find(Tsq::attr_PROFILE_NFILES)) != j) {
        params->fileLimit = strtoul(i->second.c_str(), NULL, 10);
    }
    if ((i = m_attributes.find(Tsq::attr_PROFILE_PROMPTNEWLINE)) != j) {
        params->promptNewline = (i->second == "true"s || i->second == "1"s);
    }
    if ((i = m_attributes.find(Tsq::attr_PROFILE_SCROLLCLEAR)) != j) {
        params->scrollClear = (i->second == "true"s || i->second == "1"s);
    }
    if ((i = m_attributes.find(Tsq::attr_PROFILE_ENCODING)) != j) {
        unicoding = i->second;
    }
    if ((i = m_attributes.find(Tsq::attr_PREF_LANG)) != j) {
        lang = i->second;
    }

    params->unicoding = Tsq::Unicoding::create(unicoding);
    params->translator = g_args->getTranslator(lang);
}

std::string
TermInstance::configuredStartParams(PtyParams *p) const
{
    std::string msg;
    p->env = TERM_ENVIRON;

    {
        StateLock slock(this, false);

        StringMap::const_iterator j = m_attributes.end(), i;

        if ((i = m_attributes.find(Tsq::attr_PREF_COMMAND)) != j) {
            p->command = i->second;
        } else if ((i = m_attributes.find(Tsq::attr_PROFILE_COMMAND)) != j) {
            p->command = i->second;
        } else {
            p->command = TERM_COMMAND;
        }

        if ((i = m_attributes.find(Tsq::attr_PROFILE_ENVIRON)) != j) {
            p->env.push_back('\0');
            p->env.append(i->second);
        }
        if ((i = m_attributes.find(Tsq::attr_PREF_ENVIRON)) != j) {
            p->env.push_back('\0');
            p->env.append(i->second);
        }

        if ((i = m_attributes.find(Tsq::attr_PREF_STARTDIR)) != j && !i->second.empty()) {
            p->dir = i->second;
        } else if ((i = m_attributes.find(Tsq::attr_PROFILE_STARTDIR)) != j) {
            p->dir = i->second;
        }

        if ((i = m_attributes.find(Tsq::attr_PROFILE_MESSAGE)) != j) {
            msg = i->second;
        }
        if ((i = m_attributes.find(Tsq::attr_PREF_MESSAGE)) != j) {
            msg += i->second;
        }
    }

    for (char &c: p->command)
        if (c == '\x1f')
            c = '\0';

    for (char &c: p->env)
        if (c == '\x1f')
            c = '\0';

    osRelativeToHome(p->dir);
    return msg;
}

int
TermInstance::configuredExitAction() const
{
    int result = Tsq::ExitActionStop;
    StateLock slock(this, false);

    auto i = m_attributes.find(Tsq::attr_PROFILE_EXITACTION);
    if (i != m_attributes.end())
        result = atoi(i->second.c_str());

    return result;
}

std::pair<int,int>
TermInstance::configuredAutoClose() const
{
    int time = DEFAULT_AUTOCLOSE_TIME;
    int code = Tsq::AutoCloseExit;
    StateLock slock(this, false);

    auto j = m_attributes.end();
    auto i = m_attributes.find(Tsq::attr_PROFILE_AUTOCLOSETIME);

    if (i != j)
        time = atoi(i->second.c_str());
    if ((i = m_attributes.find(Tsq::attr_PROFILE_AUTOCLOSE)) != j)
        code = atoi(i->second.c_str());

    return std::pair<int,int>(time, code);
}
