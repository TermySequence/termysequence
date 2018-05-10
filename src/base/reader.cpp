// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/config.h"
#include "app/logging.h"
#include "app/protocol.h"
#include "reader.h"
#include "listener.h"
#include "manager.h"
#include "server.h"
#include "uploadtask.h"
#include "downloadtask.h"
#include "lib/exception.h"
#include "lib/wire.h"
#include "lib/raw.h"

#include <QSocketNotifier>
#include <unistd.h>

#define TR_ERROR1 TL("error", "Unrecognized action \"%1\"")
#define TR_ERROR2 TL("error", "Server %1 not connected")

ReaderConnection::ReaderConnection() :
    QObject(g_listener)
{
    m_machine = new Tsq::RawProtocol(this);
}

ReaderConnection::~ReaderConnection()
{
    delete m_machine;
}

void
ReaderConnection::stop()
{
    if (m_fd != -1) {
        delete m_notifier;
        close(m_fd);
        m_fd = -1;
    }
    deleteLater();
}

void
ReaderConnection::start(int fd)
{
    uint32_t version = htole32(TSQT_PROTOCOL_VERSION);
    m_fd = fd;
    writeFd((const char *)&version, 4);
    m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_notifier, SIGNAL(activated(int)), SLOT(handleConnection()));
}

void
ReaderConnection::writeFd(const char *buf, size_t len)
{
    while (len) {
        ssize_t rc = write(m_fd, buf, len);
        if (rc < 0) {
            if (errno == EINTR)
                continue;

            // TODO handle EAGAIN here
            qCWarning(lcSettings, "Failed to write to local socket: %s", strerror(errno));
            throw Tsq::ErrnoException("write", errno);
        }
        len -= rc;
        buf += rc;
    }
}

void
ReaderConnection::handleConnection()
{
    try {
        m_machine->connRead(m_fd);
    } catch (const std::exception &) {
        stop();
    }
}

void
ReaderConnection::eofCallback(int)
{
    stop();
}

void
ReaderConnection::pushPipeMessage(unsigned code, const std::string &str)
{
    Tsq::ProtocolMarshaler m(m_pipeCommand);
    m.addNumber(code);
    m.addString(str);
    m_machine->connFlush(m.resultPtr(), m.length());
}

void
ReaderConnection::wirePipe(uint32_t command, const std::string &spec)
{
    QString name = QString::fromStdString(spec);
    ServerInstance *result = nullptr;
    TermManager *manager = g_listener->activeManager();

    m_pipeCommand = command;

    for (auto *server: g_listener->servers())
        if (server->shortname() == name) {
            result = server;
            break;
        }

    if (!result || !manager) {
        pushPipeMessage(1, TR_ERROR2.arg(name).toStdString());
        stop();
        return;
    }

    auto *task = (command == TSQT_UPLOAD_PIPE) ?
        (TermTask*)new UploadPipeTask(result, this) :
        (TermTask*)new DownloadPipeTask(result, this);

    task->start(manager);

    // Transfer ownership of m_fd to task
    delete m_notifier;
}

void
ReaderConnection::wireList()
{
    Tsq::ProtocolMarshaler m(TSQT_LIST_SERVERS);

    const auto *localServer = g_listener->localServer();
    if (localServer)
        m.addPaddedString(localServer->shortname().toStdString());

    for (const auto *server: g_listener->servers())
        if (!server->local())
            m.addPaddedString(server->shortname().toStdString());

    m_machine->connFlush(m.resultPtr(), m.length());
}

void
ReaderConnection::wireAction(const QByteArray &spec)
{
    Tsq::ProtocolMarshaler m(TSQT_RUN_ACTION);

    TermManager *manager = g_listener->activeManager();
    if (manager && manager->validateSlot(spec)) {
        manager->invokeSlot(spec);
        m.addNumber(0);
    } else {
        m.addNumber(1);
        m.addString(pr(TR_ERROR1.arg(QString(spec).section('|', 0, 0))));
    }

    m_machine->connFlush(m.resultPtr(), m.length());
}

bool
ReaderConnection::protocolCallback(uint32_t command, uint32_t length, const char *body)
{
    Tsq::ProtocolUnmarshaler unm(body, length);

    switch (command) {
    case TSQT_UPLOAD_PIPE:
    case TSQT_DOWNLOAD_PIPE:
        wirePipe(command, unm.parseString());
        return false;
    case TSQT_LIST_SERVERS:
        wireList();
        return true;
    case TSQT_RUN_ACTION:
        wireAction(QByteArray::fromStdString(unm.parseString()));
        return true;
    default:
        stop();
        return false;
    }
}
