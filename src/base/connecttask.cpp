// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "app/icons.h"
#include "connecttask.h"
#include "conn.h"
#include "listener.h"
#include "manager.h"
#include "term.h"
#include "connectstatus.h"
#include "settings/settings.h"
#include "settings/connect.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/attrstr.h"
#include "lib/trstr.h"

#include <cerrno>

#define TR_ERROR1 TL("error", "Connection \"%1\" required by \"%2\" not found")
#define TR_ERROR2 "Cyclical dependency detected"
#define TR_ERROR3 TL("error", "Connection %1 failed")
#define TR_TASKOBJ1 TL("task-object", "N/A")
#define TR_TASKOBJ2 TL("task-object", "Command") + ':'
#define TR_TASKOBJ3 TL("task-object", "Server") + ':'
#define TR_TASKOBJ4 TL("task-object", "PID") + ':'
#define TR_TASKOBJ5 TL("task-object", "Connection") + ':'
#define TR_TASKSTAT1 TL("task-status", "Waiting for dialog")
#define TR_TASKSTAT2 TL("task-status", "Error") + A(": ")
#define TR_TASKTYPE1 TL("task-type", "Make Connection")
#define TR_TASKTYPE2 TL("task-type", "Batch Connection")

//
// Connect task
//
ConnectTask::ConnectTask(ServerInstance *server, ConnectSettings *conninfo) :
    TermTask(server, false)
{
    m_info = conninfo;
    m_info->takeReference();
    m_info->activate();
    disableStatusPopup();

    m_typeStr = TR_TASKTYPE1;
    m_typeIcon = ICON_TASKTYPE_RUN_CONNECT;
    m_fromStr = TR_TASKOBJ1;
    m_sourceStr = TR_TASKOBJ2 + m_info->commandStr();

    // server can be null here
    memcpy(m_buf + 16, g_listener->id().buf, 16);
    memcpy(m_buf + 32, m_taskId.buf, 16);
}

ConnectTask::~ConnectTask()
{
    m_info->putReference();
}

const QString &
ConnectTask::name() const
{
    return m_info->name();
}

void
ConnectTask::start(TermManager *manager)
{
    if (TermTask::doStart(manager)) {
        // Deferred setup
        memcpy(m_buf, m_server->id().buf, 16);
        m_toStr = TR_TASKOBJ3 + m_server->shortname();

        // Write task start
        Tsq::ProtocolMarshaler m(TSQ_RUN_CONNECT);
        m.addBytes(m_buf, 48);
        m.addStringPair(Tsq::attr_COMMAND_COMMAND,
                        m_info->command().join('\x1f').toStdString());
        m.addStringPair(Tsq::attr_COMMAND_ENVIRON,
                        m_info->environ().join('\x1f').toStdString());
        m.addStringPair(Tsq::attr_COMMAND_STARTDIR,
                        m_info->directory().toStdString());
        m.addStringPair(Tsq::attr_COMMAND_PROTOCOL,
                        std::to_string(m_info->raw()));
        m.addStringPair(Tsq::attr_COMMAND_PTY,
                        std::to_string(m_info->pty()));
        m.addStringPair(Tsq::attr_COMMAND_KEEPALIVE,
                        std::to_string(m_info->keepalive()));
        m_server->conn()->push(m.resultPtr(), m.length());

        // Bring up status dialog on a timer
        m_dialog = new ConnectStatusDialog(manager->parentWidget(), m_info);
        connect(m_dialog, SIGNAL(destroyed()), SLOT(handleDialogDestroyed()));
        m_mocInput = connect(m_dialog, SIGNAL(inputRequest(QString)),
                             SLOT(handleDialogInput(const QString&)));
        m_dialog->appendOutput(m_info->commandStr() + "\n");
        m_dialog->bringUp();
    }
}

inline void
ConnectTask::killDialog()
{
    m_dialog->disconnect(this);
    m_dialog->deleteLater();
}

inline void
ConnectTask::handleError(Tsq::ProtocolUnmarshaler *unm)
{
    int errtype = unm->parseNumber();
    int errnum = unm->parseNumber();
    QString failMsg = QString::fromStdString(unm->parseString());

    switch (errtype) {
    case Tsq::ConnectTaskErrorWriteFailed:
        failMsg = TR_CONNERR3 + A(": ") + failMsg;
        break;
    case Tsq::ConnectTaskErrorRemoteReadFailed:
        failMsg = TR_CONNERR4 + A(": ") + failMsg;
        break;
    case Tsq::ConnectTaskErrorRemoteConnectFailed:
        failMsg = TR_CONNERR5;
        break;
    case Tsq::ConnectTaskErrorRemoteHandshakeFailed:
        failMsg = TR_CONNERR6.arg(errnum);
        break;
    case Tsq::ConnectTaskErrorRemoteLimitExceeded:
        failMsg = TR_CONNERR7;
        break;
    case Tsq::ConnectTaskErrorLocalReadFailed:
        failMsg = TR_CONNERR8 + A(": ") + failMsg;
        break;
    case Tsq::ConnectTaskErrorLocalConnectFailed:
        failMsg = TR_CONNERR9;
        break;
    case Tsq::ConnectTaskErrorLocalHandshakeFailed:
        failMsg = TR_CONNERR10.arg(errnum);
        break;
    case Tsq::ConnectTaskErrorLocalTransferFailed:
        failMsg = TR_CONNERR11 + A(": ") + failMsg;
        break;
    case Tsq::ConnectTaskErrorLocalRejection:
        switch (errnum) {
        case TSQ_STATUS_PROTOCOL_MISMATCH:
            failMsg = TR_EXIT6;
            break;
        case TSQ_STATUS_DUPLICATE_CONN:
            failMsg = TR_EXIT8;
            break;
        default:
            failMsg = TR_EXIT4;
        }
        break;
    case Tsq::ConnectTaskErrorLocalBadProtocol:
        failMsg = TR_CONNERR13.arg(errnum);
        break;
    case Tsq::ConnectTaskErrorLocalBadResponse:
        failMsg = TR_CONNERR14.arg(errnum);
        break;
    case Tsq::ConnectTaskErrorReadIdFailed:
        failMsg = TR_CONNERR15;
        break;
    }

    if (m_dialog->isVisible()) {
        m_failing = true;
        disconnect(m_mocInput);
        m_failMsg = failMsg;
        m_dialog->setFailure(TR_TASKSTAT2 + failMsg);
        m_dialog->show();

        setStatus(TR_TASKSTAT1, 100);
        emit taskChanged();
    }
    else {
        killDialog();
        fail(failMsg);
    }
}

inline void
ConnectTask::doFinish(ServerInstance *server)
{
    if (server->conn())
        server->setActive(m_info);

    if (m_timerId != 0)
        killTimer(m_timerId);

    disconnect(m_mocServer);
    finish();
}

void
ConnectTask::handleServerAdded(ServerInstance *server)
{
    if (server->peer() && server->peer()->id() == m_connId)
        doFinish(server);
}

void
ConnectTask::handleFinish()
{
    killDialog();

    auto *term = g_listener->lookupTerm(m_connId);
    if (term && term->peer()) {
        // We already know about this server
        doFinish(term->peer());
    } else {
        // Wait a short while for server to show up
        m_finishing = true;
        m_mocServer = connect(g_listener, SIGNAL(serverAdded(ServerInstance*)),
                              SLOT(handleServerAdded(ServerInstance*)));
        m_timerId = startTimer(TASK_CONNECT_TIME);
    }
}

inline void
ConnectTask::handleMessage(const std::string &str)
{
    m_dialog->appendOutput(QString::fromStdString(str));
    m_dialog->show();

    m_received += str.size();
    emit taskChanged();
}

void
ConnectTask::handleOutput(Tsq::ProtocolUnmarshaler *unm)
{
    if (finished() || m_finishing)
        return;

    switch (unm->parseNumber()) {
    case Tsq::TaskRunning:
        handleMessage(unm->parseString());
        break;
    case Tsq::TaskStarting:
        m_sinkStr = TR_TASKOBJ4 + QString::number(unm->parseNumber());
        emit taskChanged();
        break;
    case Tsq::TaskFinished:
        m_connId = unm->parseUuid();
        handleFinish();
        break;
    case Tsq::TaskError:
        handleError(unm);
        break;
    default:
        break;
    }
}

void
ConnectTask::cancel()
{
    if (!finished()) {
        if (m_dialog)
            killDialog();

        // Write task cancel
        Tsq::ProtocolMarshaler m(TSQ_CANCEL_TASK);
        m.addBytes(m_buf, 48);
        m_server->conn()->push(m.resultPtr(), m.length());
    }

    TermTask::cancel();
}

void
ConnectTask::handleDialogDestroyed()
{
    m_dialog = nullptr;

    if (m_failing)
        fail(m_failMsg);
    else
        cancel();
}

void
ConnectTask::handleDialogInput(const QString &input)
{
    // Write task input
    QByteArray bytes = input.toUtf8();
    Tsq::ProtocolMarshaler m(TSQ_TASK_INPUT);
    m.addBytes(m_buf, 48);
    m.addBytes(bytes.data(), bytes.size());
    m_server->conn()->push(m.resultPtr(), m.length());

    m_sent += bytes.size();
    emit taskChanged();
}

void
ConnectTask::timerEvent(QTimerEvent *)
{
    // The server announcement didn't show up
    killTimer(m_timerId);
    disconnect(m_mocServer);
    fail(strerror(ETIMEDOUT));
}

bool
ConnectTask::clonable() const
{
    return m_target;
}

TermTask *
ConnectTask::clone() const
{
    return new ConnectTask(m_server, m_info);
}

//
// Connect batcher
//
ConnectBatch::ConnectBatch(ConnectSettings *conninfo) :
    TermTask(false),
    m_info(conninfo)
{
    disableStatusPopup();

    m_typeStr = TR_TASKTYPE2;
    m_typeIcon = ICON_TASKTYPE_RUN_BATCH;
    m_toStr = m_fromStr = TR_TASKOBJ1;
    m_sourceStr = TR_TASKOBJ5 + m_info->name();
}

ConnectBatch::~ConnectBatch()
{
    for (auto i = m_map.cbegin(), j = m_map.cend(); i != j; ++i) {
        (*i)->info->putReference();
        delete *i;
    }
}

ConnectBatch::BatchEntry *
ConnectBatch::traverse(ConnectSettings *cur, QSet<QString> &seen, TermManager *manager)
{
    // Insert ourself
    cur->takeReference();
    cur->activate();
    BatchEntry *ent = new BatchEntry{ cur, cur->batch() };
    m_map[cur->name()] = ent;
    seen.insert(cur->name());

    // Traverse children
    for (const QString &name: qAsConst(ent->subs))
    {
        auto *info = g_settings->conn(name);
        if (!info) {
            failStart(manager, TR_ERROR1.arg(name, cur->name()));
            return nullptr;
        }

        if (info->isbatch()) {
            if (seen.contains(info->name())) {
                failStart(manager, TR_ERROR2);
                return nullptr;
            }
            if (!traverse(info, seen, manager)) {
                return nullptr;
            }
        }
        else if (!m_map.contains(info->name())) {
            info->takeReference();
            info->activate();
            m_map[info->name()] = new BatchEntry{ info };
            ++m_total;
        }
    }

    return ent;
}

void
ConnectBatch::start(TermManager *manager)
{
    QSet<QString> seen;

    if ((m_cur = traverse(m_info, seen, manager))) {
        TermTask::doStart(manager);
        startNext();
    }
}

void
ConnectBatch::startNext()
{
    if (m_cur->pos == m_cur->subs.size()) {
        if (m_stack.isEmpty()) {
            finish();
        } else {
            m_cur = m_stack.takeLast();
            moveNext();
        }
        return;
    }
    auto *ent = m_map[m_cur->subs[m_cur->pos]];
    if (ent->info->isbatch()) {
        m_stack.push_back(m_cur);
        m_cur = ent;
        startNext();
        return;
    }

    m_sinkStr = TR_TASKOBJ5 + ent->info->name();
    setProgress(++m_done, m_total);

    auto *manager = g_listener->activeManager();
    QObject *connobj = g_listener->launchConnection(ent->info, manager);
    ServerConnection *conn = qobject_cast<ServerConnection*>(connobj);
    ConnectTask *task = qobject_cast<ConnectTask*>(connobj);

    if (conn) {
        if (conn->populating()) {
            connect(conn, SIGNAL(ready()), SLOT(handleConnectionReady()));
            connect(conn, SIGNAL(connectionFailed()), SLOT(handleConnectionFailed()));
        } else {
            moveNext();
        }
    }
    else if (task->finished()) {
        handleTask(task);
    }
    else {
        connect(task, SIGNAL(taskChanged()), SLOT(handleTaskChanged()));
    }
}

void
ConnectBatch::moveNext()
{
    ++m_cur->pos;
    startNext();
}

void
ConnectBatch::handleConnectionReady()
{
    if (!finished()) {
        sender()->disconnect(this);
        moveNext();
    }
}

void
ConnectBatch::handleConnectionFailed()
{
    if (!finished()) {
        auto *conn = static_cast<ServerConnection*>(sender());
        conn->disconnect(this);
        fail(TR_ERROR3.arg(conn->name()) + A(": ") + conn->failureMessage());
    }
}

void
ConnectBatch::handleTask(ConnectTask *task)
{
    if (task->finished()) {
        task->disconnect(this);
        if (task->succeeded())
            moveNext();
        else
            fail(TR_ERROR3.arg(task->name()) + A(": ") + task->statusStr());
    }
}

void
ConnectBatch::handleTaskChanged()
{
    if (!finished())
        handleTask(static_cast<ConnectTask*>(sender()));
}

void
ConnectBatch::cancel()
{
    TermTask::cancel();
}

bool
ConnectBatch::clonable() const
{
    return true;
}

TermTask *
ConnectBatch::clone() const
{
    return new ConnectBatch(m_info);
}
