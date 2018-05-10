// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/icons.h"
#include "uploadtask.h"
#include "conn.h"
#include "reader.h"
#include "listener.h"
#include "settings/servinfo.h"
#include "settings/global.h"
#include "os/fd.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/sequences.h"

#include <QSocketNotifier>

#include <cerrno>
#include <exception>
#include <unistd.h>

#define TR_ASK1 TL("question", "File exists. Overwrite?")
#define TR_TASKOBJ1 TL("task-object", "This computer")
#define TR_TASKOBJ2 TL("task-object", "File") + ':'
#define TR_TASKOBJ3 TL("task-object", "Pipe client")
#define TR_TASKOBJ4 TL("task-object", "Pipe") + ':'
#define TR_TASKTYPE1 TL("task-type", "Upload File")
#define TR_TASKTYPE2 TL("task-type", "Pipe To")

#define BUFSIZE (16 * TERM_PAYLOADSIZE)
#define HEADERSIZE 56
#define PAYLOADSIZE (BUFSIZE - HEADERSIZE)
#define WINDOWSIZE 8

//
// Upload to file
//
inline void
UploadFileTask::setup(bool overwrite)
{
    m_fromStr = TR_TASKOBJ1;

    uint32_t command = htole32(TSQ_TASK_INPUT);

    m_buf = new char[BUFSIZE];
    memcpy(m_buf, &command, 4);
    memcpy(m_buf + 8, m_server->id().buf, 16);
    memcpy(m_buf + 24, g_listener->id().buf, 16);
    memcpy(m_buf + 40, m_taskId.buf, 16);
    m_ptr = m_buf + HEADERSIZE;

    if (overwrite)
        m_config = Tsq::TaskOverwrite;
    else if (m_server->serverInfo()->uploadConfig() >= 0)
        m_config = m_server->serverInfo()->uploadConfig();
    else
        m_config = g_global->uploadConfig();
}

UploadFileTask::UploadFileTask(ServerInstance *server, const QString &infile,
                               const QString &outfile, bool overwrite) :
    TermTask(server, false),
    m_overwrite(overwrite),
    m_infile(infile),
    m_outfile(outfile)
{
    m_typeStr = TR_TASKTYPE1;
    m_typeIcon = ICON_TASKTYPE_UPLOAD_FILE;
    m_sourceStr = TR_TASKOBJ2 + infile;
    m_sinkStr = TR_TASKOBJ2 + outfile;

    setup(overwrite);
}

UploadFileTask::UploadFileTask(ServerInstance *server, int fd) :
    TermTask(server, true)
{
    m_fd = fd;

    m_typeStr = TR_TASKTYPE2;
    m_typeIcon = ICON_TASKTYPE_UPLOAD_PIPE;
    m_sourceStr = TR_TASKOBJ3;

    setup(true);
}

inline void
UploadFileTask::closefd()
{
    if (m_fd != -1)
    {
        m_running = false;
        m_notifier->setEnabled(false);
        close(m_fd);
        m_fd = -1;
    }
}

UploadFileTask::~UploadFileTask()
{
    closefd();
    delete [] m_buf;
}

void
UploadFileTask::pushCancel(int code)
{
    Tsq::ProtocolMarshaler m(TSQ_CANCEL_TASK);
    m.addBytes(m_buf + 8, 48);
    m_server->conn()->push(m.resultPtr(), m.length());

    if (code)
        fail(strerror(code));
}

void
UploadFileTask::start(TermManager *manager)
{
    uint32_t mode;

    try {
        m_fd = osOpenFile(pr(m_infile), &m_total, &mode);
    } catch (const std::exception &e) {
        failStart(manager, e.what());
        return;
    }

    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &UploadFileTask::handleFile);
    m_notifier->setEnabled(false);

    if (TermTask::doStart(manager)) {
        // Write task start
        Tsq::ProtocolMarshaler m(TSQ_UPLOAD_FILE);
        m.addBytes(m_buf + 8, 48);
        m.addNumberPair(PAYLOADSIZE, mode);
        m.addNumber(m_config);
        m.addBytes(m_outfile.toStdString());

        m_server->conn()->push(m.resultPtr(), m.length());
    } else {
        closefd();
    }
}

void
UploadFileTask::handleFile(int fd)
{
    if (m_sent - m_acked >= WINDOWSIZE * PAYLOADSIZE) {
        // Stop and wait for ack message
        m_running = false;
        m_notifier->setEnabled(false);
    }
    else {
        ssize_t rc = read(fd, m_ptr, PAYLOADSIZE);
        if (rc >= 0) {
            uint32_t length = htole32(HEADERSIZE - 8 + rc);
            memcpy(m_buf + 4, &length, 4);
            m_server->conn()->push(m_buf, HEADERSIZE + rc);
            setProgress(m_sent += rc, m_total);

            if (rc == 0)
                closefd();
        } else {
            pushCancel(errno);
            closefd();
        }
    }
}

void
UploadFileTask::handleStart(const std::string &name)
{
    m_sinkStr = TR_TASKOBJ2 + QString::fromStdString(name);
}

void
UploadFileTask::handleOutput(Tsq::ProtocolUnmarshaler *unm)
{
    if (finished())
        return;

    int status = unm->parseNumber();
    size_t acked = unm->parseNumber64();

    switch (status) {
    case Tsq::TaskAcking:
        m_acked = acked;
        m_running = true;
        m_notifier->setEnabled(!m_throttled);
        break;
    case Tsq::TaskStarting:
        handleStart(unm->parseString());
        emit taskChanged();
        break;
    case Tsq::TaskFinished:
        finish();
        break;
    case Tsq::TaskError:
        closefd();
        unm->parseNumber();
        fail(QString::fromStdString(unm->parseString()));
        break;
    default:
        break;
    }
}

void
UploadFileTask::handleQuestion(int question)
{
    if (finished() || question != Tsq::TaskOverwriteRenameQuestion)
        return;

    setQuestion(Tsq::TaskOverwriteRenameQuestion, TR_ASK1);
}

void
UploadFileTask::handleAnswer(int answer)
{
    if (!finished()) {
        Tsq::ProtocolMarshaler m(TSQ_TASK_ANSWER);
        m.addBytes(m_buf + 8, 48);
        m.addNumber(answer);

        m_server->conn()->push(m.resultPtr(), m.length());
        clearQuestion();
    }
}

void
UploadFileTask::cancel()
{
    closefd();

    // Write task cancel
    if (!finished())
        pushCancel(0);

    TermTask::cancel();
}

void
UploadFileTask::handleDisconnect()
{
    closefd();
}

void
UploadFileTask::handleThrottle(bool throttled)
{
    m_throttled = throttled;
    m_notifier->setEnabled(!throttled && m_running);
}

bool
UploadFileTask::clonable() const
{
    return finished() && m_target;
}

TermTask *
UploadFileTask::clone() const
{
    return new UploadFileTask(m_server, m_infile, m_outfile, m_overwrite);
}

//
// Upload to pipe
//
UploadPipeTask::UploadPipeTask(ServerInstance *server, ReaderConnection *reader) :
    UploadFileTask(server, reader->fd()),
    m_reader(reader)
{
    reader->setParent(this);
}

void
UploadPipeTask::start(TermManager *manager)
{
    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &UploadPipeTask::handleFile);
    m_notifier->setEnabled(false);

    if (TermTask::doStart(manager)) {
        // Write task start
        Tsq::ProtocolMarshaler m(TSQ_UPLOAD_PIPE);
        m.addBytes(m_buf + 8, 48);
        m.addNumberPair(PAYLOADSIZE, 0644);

        m_server->conn()->push(m.resultPtr(), m.length());
    } else {
        closefd();
    }
}

void
UploadPipeTask::handleFile(int fd)
{
    if (m_sent - m_acked >= WINDOWSIZE * PAYLOADSIZE) {
        // Stop and wait for ack message
        m_running = false;
        m_notifier->setEnabled(false);
    }
    else {
        ssize_t rc = read(fd, m_ptr, PAYLOADSIZE);
        if (rc >= 0) {
            uint32_t length = htole32(HEADERSIZE - 8 + rc);
            memcpy(m_buf + 4, &length, 4);
            m_server->conn()->push(m_buf, HEADERSIZE + rc);
            m_sent += rc;
            queueTaskChange();

            if (rc == 0)
                closefd();
        } else {
            pushCancel(errno);
            closefd();
        }
    }
}

void
UploadPipeTask::handleStart(const std::string &name)
{
    m_reader->pushPipeMessage(0, name);
    m_sinkStr = TR_TASKOBJ4 + QString::fromStdString(name);
}

void
UploadPipeTask::handleQuestion(int)
{}

bool
UploadPipeTask::clonable() const
{
    return false;
}
