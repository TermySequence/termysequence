// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/icons.h"
#include "misctask.h"
#include "conn.h"
#include "listener.h"
#include "settings/global.h"
#include "settings/servinfo.h"
#include "lib/protocol.h"
#include "lib/wire.h"

#define TR_ASK1 TL("question", "Really remove file?")
#define TR_ASK2 TL("question", "Remove folder and all its contents?")
#define TR_ASK3 TL("question", "Destination file exists. Overwrite?")
#define TR_TASKOBJ1 TL("task-object", "N/A")
#define TR_TASKOBJ2 TL("task-object", "File") + ':'
#define TR_TASKTYPE1 TL("task-type", "Delete File")
#define TR_TASKTYPE2 TL("task-type", "Rename File")

//
// Delete file task
//
DeleteFileTask::DeleteFileTask(ServerInstance *server, const QString &fileName) :
    TermTask(server, false),
    m_fileName(fileName)
{
    m_typeStr = TR_TASKTYPE1;
    m_typeIcon = ICON_TASKTYPE_DELETE_FILE;
    m_sourceStr = m_fromStr = TR_TASKOBJ1;
    m_sinkStr = TR_TASKOBJ2 + fileName;

    memcpy(m_buf, server->id().buf, 16);
    memcpy(m_buf + 16, g_listener->id().buf, 16);
    memcpy(m_buf + 32, m_taskId.buf, 16);

    if (server->serverInfo()->deleteConfig() >= 0)
        m_config = server->serverInfo()->deleteConfig();
    else
        m_config = g_global->deleteConfig();
}

void
DeleteFileTask::start(TermManager *manager)
{
    if (TermTask::doStart(manager)) {
        // Write task start
        Tsq::ProtocolMarshaler m(TSQ_DELETE_FILE);
        m.addBytes(m_buf, 48);
        m.addNumber(m_config);
        m.addBytes(m_fileName.toStdString());

        m_server->conn()->push(m.resultPtr(), m.length());
    }
}

void
DeleteFileTask::handleOutput(Tsq::ProtocolUnmarshaler *unm)
{
    if (finished())
        return;

    switch (unm->parseNumber()) {
    case Tsq::TaskFinished:
        finish();
        break;
    case Tsq::TaskError:
        unm->parseNumber();
        fail(QString::fromStdString(unm->parseString()));
        break;
    default:
        break;
    }
}

void
DeleteFileTask::handleQuestion(int question)
{
    if (finished())
        return;

    switch (question) {
    case Tsq::TaskRemoveQuestion:
        setQuestion(Tsq::TaskRemoveQuestion, TR_ASK1);
        break;
    case Tsq::TaskRecurseQuestion:
        setQuestion(Tsq::TaskRecurseQuestion, TR_ASK2);
        break;
    default:
        break;
    }
}

void
DeleteFileTask::handleAnswer(int answer)
{
    if (!finished()) {
        Tsq::ProtocolMarshaler m(TSQ_TASK_ANSWER);
        m.addBytes(m_buf, 48);
        m.addNumber(answer);

        m_server->conn()->push(m.resultPtr(), m.length());
        clearQuestion();
    }
}

void
DeleteFileTask::cancel()
{
    // Write task cancel
    if (!finished()) {
        Tsq::ProtocolMarshaler m(TSQ_CANCEL_TASK);
        m.addBytes(m_buf, 48);
        m_server->conn()->push(m.resultPtr(), m.length());
    }

    TermTask::cancel();
}

bool
DeleteFileTask::clonable() const
{
    return finished() && m_target;
}

TermTask *
DeleteFileTask::clone() const
{
    return new DeleteFileTask(m_server, m_fileName);
}

//
// Rename file task
//
RenameFileTask::RenameFileTask(ServerInstance *server, const QString &infile,
                               const QString &outfile) :
    TermTask(server, false),
    m_infile(infile),
    m_outfile(outfile)
{
    m_typeStr = TR_TASKTYPE2;
    m_typeIcon = ICON_TASKTYPE_RENAME_FILE;
    m_fromStr = TR_TASKOBJ1;
    m_sourceStr = TR_TASKOBJ2 + infile;
    m_sinkStr = TR_TASKOBJ2 + outfile;

    memcpy(m_buf, server->id().buf, 16);
    memcpy(m_buf + 16, g_listener->id().buf, 16);
    memcpy(m_buf + 32, m_taskId.buf, 16);

    if (server->serverInfo()->renameConfig() >= 0)
        m_config = server->serverInfo()->renameConfig();
    else
        m_config = g_global->renameConfig();
}

void
RenameFileTask::start(TermManager *manager)
{
    if (TermTask::doStart(manager)) {
        // Write task start
        Tsq::ProtocolMarshaler m(TSQ_RENAME_FILE);
        m.addBytes(m_buf, 48);
        m.addNumber(m_config);
        m.addStringPair(m_infile.toStdString(), m_outfile.toStdString());

        m_server->conn()->push(m.resultPtr(), m.length());
    }
}

void
RenameFileTask::handleOutput(Tsq::ProtocolUnmarshaler *unm)
{
    if (finished())
        return;

    switch (unm->parseNumber()) {
    case Tsq::TaskFinished:
        finish();
        break;
    case Tsq::TaskError:
        unm->parseNumber();
        fail(QString::fromStdString(unm->parseString()));
        break;
    default:
        break;
    }
}

void
RenameFileTask::handleQuestion(int question)
{
    if (finished() || question != Tsq::TaskOverwriteQuestion)
        return;

    setQuestion(Tsq::TaskOverwriteQuestion, TR_ASK3);
}

void
RenameFileTask::handleAnswer(int answer)
{
    if (!finished()) {
        Tsq::ProtocolMarshaler m(TSQ_TASK_ANSWER);
        m.addBytes(m_buf, 48);
        m.addNumber(answer);

        m_server->conn()->push(m.resultPtr(), m.length());
        clearQuestion();
    }
}

void
RenameFileTask::cancel()
{
    // Write task cancel
    if (!finished()) {
        Tsq::ProtocolMarshaler m(TSQ_CANCEL_TASK);
        m.addBytes(m_buf, 48);
        m_server->conn()->push(m.resultPtr(), m.length());
    }

    TermTask::cancel();
}

bool
RenameFileTask::clonable() const
{
    return finished() && m_target;
}

TermTask *
RenameFileTask::clone() const
{
    return new RenameFileTask(m_server, m_infile, m_outfile);
}
