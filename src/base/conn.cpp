// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/exception.h"
#include "app/attrbase.h"
#include "app/logging.h"
#include "app/reaper.h"
#include "conn.h"
#include "listener.h"
#include "term.h"
#include "connectstatus.h"
#include "settings/connect.h"
#include "settings/state.h"
#include "os/dir.h"
#include "os/process.h"
#include "os/pty.h"
#include "os/signal.h"
#include "lib/protocol.h"
#include "lib/handshake.h"
#include "lib/raw.h"
#include "lib/term.h"
#include "lib/trstr.h"
#include "config.h"

#include <QSocketNotifier>
#include <unistd.h>

#define TR_ERROR1 TL("error", "The %1 executable was not found on the PATH")
#define TR_TASKSTAT1 TL("task-status", "Handshake failed")
#define TR_TASKSTAT2 TL("task-status", "Remote end closed")
#define TR_TASKSTAT3 TL("task-status", "Canceled")
#define TR_TASKSTAT4 TL("task-status", "Error") + A(": ")

ServerConnection::ServerConnection(ConnectSettings *conninfo) :
    ServerInstance(this)
{
    m_conninfo = conninfo;
    m_conninfo->takeReference();
    m_conninfo->activate();
}

ServerConnection::~ServerConnection()
{
    // m_conninfo handled in ~ServerInstance()
    if (m_fd != -1)
        close(m_fd);

    delete m_machine;
    delete m_handshake;
}

inline void
ServerConnection::closefd()
{
    if (m_fd != -1) {
        delete m_readNotifier;
        delete m_writeNotifier;
        close(m_fd);
        m_fd = -1;
    }
}

const QString &
ServerConnection::name() const
{
    return m_conninfo->name();
}

void
ServerConnection::start(bool transient)
{
    try {
        QString sdir = qApp->property(OBJPROP_SDIR).toString();
        int pid;
        m_fd = osForkServer(pr(sdir), transient, &pid);
        if (pid)
            ReaperThread::launchReaper(pid);
        if (m_fd == -1) {
            reportConnectionFailed(TR_TASKSTAT4 + TR_ERROR1.arg(SERVER_NAME));
            return;
        }

        m_transient = transient;
        m_persistent = !transient;

        m_handshake = new Tsq::ClientHandshake(g_listener->id().buf, false);
        m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        m_mocActivated = connect(m_readNotifier, SIGNAL(activated(int)),
                                 SLOT(handleHandshake(int)));

        m_writeNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Write, this);
        connect(m_writeNotifier, SIGNAL(activated(int)), SLOT(handleWrite(int)));
        m_writeNotifier->setEnabled(false);
    }
    catch (const std::exception &e) {
        reportConnectionFailed(TR_TASKSTAT4 + e.what());
    }
}

void
ServerConnection::start(QWidget *parent)
{
    try {
        PtyParams params{};

        const auto &command = m_conninfo->command();
        for (int i = 0; i < command.size(); ++i) {
            params.command.append(command.at(i).toStdString());
            if (i != command.size() - 1)
                params.command.push_back('\0');
        }

        const auto &env = m_conninfo->environ();
        for (int i = 0; i < env.size(); ++i) {
            params.env.append(env.at(i).toStdString());
            params.env.push_back('\0');
        }

        params.dir = m_conninfo->directory().toStdString();
        osRelativeToHome(params.dir);
        params.daemon = true;

        if (m_conninfo->pty()) {
            params.env.append("+TERM=dumb");
            m_fd = osForkTerminal(params, &m_pid);
        } else {
            params.env.append("-TERM");
            m_fd = osForkProcess(params, &m_pid);
        }
        ReaperThread::launchReaper(m_pid);

        m_independent = true;
        m_pty = m_conninfo->pty();
        m_raw = m_conninfo->raw();
        m_keepalive = m_conninfo->keepalive();

        m_handshake = new Tsq::ClientHandshake(g_listener->id().buf, false);
        m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        m_mocActivated = connect(m_readNotifier, SIGNAL(activated(int)),
                                 SLOT(handleHandshake(int)));

        m_writeNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Write, this);
        connect(m_writeNotifier, SIGNAL(activated(int)), SLOT(handleWrite(int)));
        m_writeNotifier->setEnabled(false);

        // Bring up status dialog on a timer
        m_dialog = new ConnectStatusDialog(parent, m_conninfo);
        connect(m_dialog, SIGNAL(destroyed()), SLOT(handleDialogDestroyed()));
        connect(m_dialog, SIGNAL(inputRequest(QString)), SLOT(handleDialogInput(const QString&)));
        m_dialog->appendOutput(m_conninfo->commandStr() + "\n");
        m_dialog->bringUp();
        // Note: past this point, don't read config values from conninfo
    }
    catch (const std::exception &e) {
        reportConnectionFailed(TR_TASKSTAT4 + e.what());
    }
}

inline void
ServerConnection::handleHandshakeHelper(const char *buf, size_t len)
{
    QByteArray output;

    for (size_t i = 0; i < len; ++i)
    {
        int rc = m_handshake->processHelloByte(buf[i]);

        if (rc == Tsq::ShakeSuccess) {
            int protocolType;
            int clientVersion = CLIENT_VERSION;

            if (m_handshake->protocolVersion != TSQ_PROTOCOL_VERSION) {
                protocolType = TSQ_PROTOCOL_REJECT;
                clientVersion = TSQ_STATUS_PROTOCOL_MISMATCH;
            } else if (m_raw) {
                protocolType = TSQ_PROTOCOL_RAW;
                m_machine = new Tsq::RawProtocol(this);
            } else {
                protocolType = TSQ_PROTOCOL_TERM;
                m_machine = new Tsq::TermProtocol(this);
            }

            if (m_pty) {
                // Place terminal in raw mode
                osMakeRawTerminal(m_fd);
            }
            disconnect(m_mocActivated);
            auto response = m_handshake->getResponse(clientVersion, protocolType);
            writeFd(response.data(), response.size());

            if (protocolType == TSQ_PROTOCOL_REJECT)
                throw TsqException("Unsupported server protocol version %u",
                                   m_handshake->protocolVersion);

            if (m_dialog) {
                m_dialog->disconnect(this);
                m_dialog->deleteLater();
                m_dialog = nullptr;
            }
            connect(m_readNotifier, SIGNAL(activated(int)), SLOT(handleConnection(int)));
            m_conninfo->setActive(this);
            return;
        }
        else if (rc == Tsq::ShakeOngoing) {
            continue;
        }
        else if (rc == Tsq::ShakeBadLeadingContent && m_dialog) {
            output.append(m_handshake->leadingContent, m_handshake->leadingContentLen);
            continue;
        }

        throw TsqException("Received bad handshake message from server (code %d)", rc);
    }

    if (!output.isEmpty()) {
        m_dialog->appendOutput(output);
        m_dialog->show();
    }
}

void
ServerConnection::handleHandshake(int fd)
{
    char buf[256];
    ssize_t rc;

    rc = read(fd, buf, sizeof(buf));
    if (rc == 0) {
        closefd();
        reportConnectionFailed(TR_TASKSTAT1);
        return;
    }
    else if (rc < 0) {
        reportConnectionFailed(TR_TASKSTAT4 + strerror(errno));
        closefd(); // errno
        return;
    }

    try {
        handleHandshakeHelper(buf, rc);
    } catch (const std::exception &e) {
        closefd();
        reportConnectionFailed(TR_TASKSTAT4 + e.what());
    }
}

void
ServerConnection::handleConnection(int fd)
{
    try {
        m_machine->connRead(fd);
        m_idleCount = 0;
    } catch (const std::exception &e) {
        stop();
        reportConnectionFailed(TR_TASKSTAT4 + e.what());
    }
}

void
ServerConnection::eofCallback(int errnum)
{
    stop();
    reportConnectionFailed(errnum ?
                           TR_TASKSTAT4 + strerror(errnum) :
                           TR_TASKSTAT2);
}

void
ServerConnection::writeFd(const char *buf, size_t len)
{
    // NOTE: do not throw from here
    if (!m_outdata.empty()) {
    push:
        // TODO notify user when data is piling up
        m_outdata.emplace(buf, len);
        m_writeNotifier->setEnabled(true);
        return;
    }
    while (len) {
        ssize_t rc = write(m_fd, buf, len);
        if (rc < 0) {
            if (errno == EINTR)
                continue;
            // Punt errors as well as EAGAIN
            goto push;
        }
        len -= rc;
        buf += rc;
    }
}

void
ServerConnection::handleWrite(int fd)
{
    while (!m_outdata.empty()) {
        QByteArray &data = m_outdata.front();
        size_t len = data.size();
        ssize_t rc = write(fd, data.data(), len);

        if (rc == len) {
            m_outdata.pop();
        }
        else if (rc < 0) {
            if (errno == EAGAIN || errno == EINTR)
                return;

            reportConnectionFailed(TR_TASKSTAT4 + strerror(errno));
            closefd(); // errno
            return;
        }
        else {
            data.remove(0, rc);
            return;
        }
    }
    m_writeNotifier->setEnabled(false);
}

void
ServerConnection::reportConnectionFailed(const QString &message)
{
    m_failed = true;
    m_failureMessage = message;

    if (m_dialog) {
        m_dialog->disconnect(this);
        m_dialog->setFailure(m_failureMessage);
        m_dialog->show();
        m_dialog = nullptr;
        m_dialogShown = true;
    }

    if (m_populating)
        emit connectionFailed();
    else
        g_listener->reportConnectionLost(this);
}

void
ServerConnection::addTerm(TermInstance *term)
{
    m_terms[term->id()] = term;
    g_listener->addTerm(term);
    // Note: order this last due to ready trigger
    term->server()->addTerm(term);
}

void
ServerConnection::removeTerm(TermInstance *term, bool remote)
{
    if (remote && term->peer() && term->isTerm())
        qCCritical(lcCommand, "Removing a terminal with an active peer");

    term->server()->removeTerm(term); // deletes
    throttleResume(term, false);
    m_throttles.remove(term);
    m_terms.remove(term->id());
}

void
ServerConnection::addServer(ServerInstance *server)
{
    m_servers[server->id()] = server;
    g_listener->addServer(server);
}

void
ServerConnection::removeServer(ServerInstance *server, bool remote)
{
    const auto &terms = server->terms();
    if (!terms.isEmpty())
    {
        if (remote)
            qCCritical(lcCommand, "Removing a server with active terminals still present");

        for (auto term: terms) {
            removeTerm(term, remote);
        }
    }

    server->setInactive();
    g_listener->removeServer(server);
    m_throttles.remove(server);
    m_servers.remove(server->id());
    server->deleteLater();
}

void
ServerConnection::stopServer(ServerInstance *server)
{
    server->clearConnection();

    for (auto &set: m_throttles)
        for (auto i = set.begin(); i != set.end(); )
            if ((*i)->server() == server)
                i = set.erase(i);
            else
                ++i;

    for (auto i = m_throttles.begin(); i != m_throttles.end(); )
        if (i->isEmpty()) {
            i.key()->setThrottled(false);
            i = m_throttles.erase(i);
        } else {
            ++i;
        }

    for (auto term: server->terms()) {
        m_throttles.remove(term);
        term->setThrottled(false);
        m_terms.remove(term->id());
    }
    m_throttles.remove(server);
    server->setThrottled(false);
    m_servers.remove(server->id());
}

void
ServerConnection::stop()
{
    closefd();
    killTimer(m_timerId);

    for (auto server: qAsConst(m_servers)) {
        server->clearConnection();
    }

    for (auto i = m_throttles.begin(), j = m_throttles.end(); i != j; ++i) {
        i.key()->setThrottled(false);
    }

    m_throttles.clear();
}

void
ServerConnection::throttleResume(TermInstance *term, bool notify)
{
    for (auto i = m_throttles.begin(); i != m_throttles.end(); ) {
        auto k = i->constFind(term);
        if (k != i->cend()) {
            i->erase(k);

            if (i->isEmpty()) {
                if (notify || i.key() != term) {
                    i.key()->setThrottled(false);
                }

                i = m_throttles.erase(i);
                continue;
            }
        }
        ++i;
    }
}

void
ServerConnection::handleDialogDestroyed()
{
    m_dialog = nullptr;

    if (m_pid)
        osKillProcess(m_pid);

    closefd();
    m_dialogShown = true;
    reportConnectionFailed(TR_TASKSTAT3);
}

void
ServerConnection::handleDialogInput(const QString &input)
{
    QByteArray bytes = input.toUtf8();
    writeFd(bytes.data(), bytes.size());
}

void
ServerConnection::timerEvent(QTimerEvent *)
{
    if (m_idleCount < 2) {
        ++m_idleCount;
    } else {
        stop();
        reportConnectionFailed(TR_EXIT11);
    }
}
