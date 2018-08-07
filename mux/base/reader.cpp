// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "reader.h"
#include "writer.h"
#include "raw.h"
#include "term.h"
#include "termwatch.h"
#include "listener.h"
#include "servermachine.h"
#include "exception.h"
#include "os/conn.h"
#include "os/logging.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/sequences.h"
#include "config.h"

#include <unistd.h>

TermReader::TermReader(int writeFd, StringMap &&environ) :
    ThreadBase("reader", ThreadBaseFd),
    m_writer(new TermWriter(this)),
    m_machine(new ServerMachine(this)),
    m_writeFd(writeFd),
    m_environ(new StringMap(environ))
{
}

TermReader::~TermReader()
{
    m_watches.insert(m_writer->m_watches.begin(), m_writer->m_watches.end());
    for (auto watch: m_watches)
        watch->release();

    if (m_savedFd != -1) {
        close(m_savedFd);
        if (m_savedWriteFd != m_savedFd)
            close(m_savedWriteFd);
    }

    delete m_machine;
    delete m_writer;
}

/*
 * Listener thread
 */
void
TermReader::setWatches(std::set<BaseWatch*,WatchSorter> &watches)
{
    m_writer->setWatches(watches);

    Lock lock(this);
    m_watches = std::move(watches);

    for (auto i = m_watches.crbegin(), j = m_watches.crend(); i != j; ++i)
        stageWork(ReaderWatchAdded, *i);

    commitWork();
}

void
TermReader::addWatch(BaseWatch *watch)
{
    m_writer->addWatch(watch);

    FlexLock lock(this);
    m_watches.emplace(watch);
    sendWorkAndUnlock(lock, ReaderWatchAdded, watch);
}

/*
 * This thread
 */
void
TermReader::closefd()
{
    if (m_fd != -1) {
        close(m_fd);
        if (m_writeFd != m_fd)
            close(m_writeFd);
        setfd(-1);
    }
}

bool
TermReader::handleFd()
{
    if (m_machine->connRead(m_fd))
        return true;
    else {
        m_cleanExit = true;
        return false;
    }
}

void
TermReader::handleWatchAdded(BaseWatch *watch)
{
    if (watch->type == WatchTerm || watch->type == WatchConn)
        m_terms.emplace(watch->parentId(), watch);

    watch->start();
}

void
TermReader::handleWatchRelease(BaseWatch *watch)
{
    bool found;

    {
        Lock lock(this);
        found = m_watches.erase(watch);
    }

    if (found) {
        if (watch->type == WatchTerm || watch->type == WatchConn)
            m_terms.erase(watch->parentId());

        watch->putReaderReference();
    }
}

bool
TermReader::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case ReaderClose:
        m_exitStatus = item.value;
        // bail out of loop
        return false;
    case ReaderWatchAdded:
        handleWatchAdded((BaseWatch*)item.value);
        break;
    case ReaderReleaseWatch:
        handleWatchRelease((BaseWatch*)item.value);
        break;
    case ReaderPostConfirm:
        m_writer->start(-1);
        break;
    default:
        break;
    }

    return true;
}

bool
TermReader::handleIdle()
{
    if (m_idleOut) {
        LOGDBG("Reader %p: keepalive timed out\n", this);
        m_exitStatus = TSQ_STATUS_IDLE_TIMEOUT;
        // bail out of loop
        return false;
    }
    else {
        std::string keepalive(TSQ_RAW_KEEPALIVE, TSQ_RAW_KEEPALIVE_LEN);
        m_writer->submitResponse(std::move(keepalive));
        m_idleOut = true;
        return true;
    }
}

void
TermReader::threadMain()
{
    try {
        m_machine->start();
        runDescriptorLoop();
    }
    catch (const TsqException &e) {
        LOGWRN("Reader %p: %s\n", this, e.what());
        m_exitStatus = e.status();
    } catch (const std::exception &e) {
        LOGERR("Reader %p: caught exception: %s\n", this, e.what());
        m_exitStatus = TSQ_STATUS_SERVER_ERROR;
    }

    if (m_writer->started()) {
        LOGDBG("Reader %p: waiting for writer %p\n", this, m_writer);
        m_writer->stop(0);
        m_writer->join();
    }

    disconnect();

    if (!m_cleanExit) {
        Tsq::ProtocolMarshaler m(TSQ_DISCONNECT);
        m.addNumber(m_exitStatus);
        std::string encoded = m_machine->encode(m.resultPtr(), m.length());
        write(m_writeFd, encoded.data(), encoded.size());
    }
    if (m_savedWriteFd != -1) {
        char c = m_exitStatus;
        write(m_savedWriteFd, &c, 1);
    }

    LOGDBG("Reader %p: goodbye\n", this);
    closefd();

    g_listener->sendWork(ListenerRemoveReader, this);
}

/*
 * State machine
 */
void
TermReader::writeFd(const char *buf, size_t len)
{
    m_writer->writeFd(m_writeFd, buf, len);
}

bool
TermReader::setMachine(Tsq::ProtocolMachine *newMachine, char protocolType)
{
    delete m_machine;
    m_machine = newMachine;

    if (m_savedWriteFd != -1)
        write(m_savedWriteFd, &protocolType, 1);

    if (!m_machine->start())
        return false;

    if (g_listener->nReaders() >= MAX_CONNECTIONS) {
        pushDisconnect(TSQ_STATUS_CONN_LIMIT_REACHED);
        m_writer->start(-1);
    } else if (g_listener->knownClient(m_remoteId)) {
        pushDisconnect(TSQ_STATUS_DUPLICATE_CONN);
        m_writer->start(-1);
    } else {
        g_listener->sendWork(ListenerConfirmReader, this);
        // writer started in PostConfirm
    }

    return true;
}

void
TermReader::setFd(int newrd, int newwd)
{
    m_savedFd = m_fd;
    m_savedWriteFd = m_writeFd;
    setfd(newrd);
    m_writeFd = newwd;
}

void
TermReader::createConn(int protocolType, const char *buf, size_t len)
{
    RawInstance *conn = new RawInstance(protocolType, m_fd, buf, len);
    LOGDBG("Reader %p: converting to Conn %p (direct)\n", this, conn);
    g_listener->sendWork(ListenerAddConn, conn);

    // transfer ownership of m_fd to conn
    setfd(-1);
}

void
TermReader::createConn(int protocolType, int newFd)
{
    RawInstance *conn = new RawInstance(protocolType, m_fd, newFd);
    LOGDBG("Reader %p: converting to Conn %p (indirect)\n", this, conn);
    g_listener->sendWork(ListenerAddConn, conn);

    // transfer ownership of m_fd to conn
    setfd(-1);
}
