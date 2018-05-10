// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/icons.h"
#include "downloadtask.h"
#include "conn.h"
#include "reader.h"
#include "listener.h"
#include "settings/servinfo.h"
#include "settings/global.h"
#include "os/fd.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/sequences.h"

#include <QUrl>
#include <QMimeData>
#include <QSocketNotifier>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>

#define TR_ASK1 TL("question", "File exists. Overwrite?")
#define TR_TASKOBJ1 TL("task-object", "This computer")
#define TR_TASKOBJ2 TL("task-object", "File") + ':'
#define TR_TASKOBJ3 TL("task-object", "Clipboard");
#define TR_TASKOBJ4 TL("task-object", "Pipe client")
#define TR_TASKOBJ5 TL("task-object", "Pipe") + ':'
#define TR_TASKTYPE1 TL("task-type", "Download File")
#define TR_TASKTYPE2 TL("task-type", "Download to Clipboard")
#define TR_TASKTYPE3 TL("task-type", "Pipe From")

#define BUFSIZE (16 * TERM_PAYLOADSIZE)
#define HEADERSIZE 60
#define PAYLOADSIZE (BUFSIZE - HEADERSIZE)
#define WINDOWSIZE 8

//
// Base class
//
inline void
DownloadTask::setup()
{
    m_toStr = TR_TASKOBJ1;

    memcpy(m_buf, m_target->id().buf, 16);
    memcpy(m_buf + 16, g_listener->id().buf, 16);
    memcpy(m_buf + 32, m_taskId.buf, 16);
}

DownloadTask::DownloadTask(ServerInstance *server, const QString &infile) :
    TermTask(server, false),
    m_infile(infile)
{
    setup();
}

DownloadTask::DownloadTask(TermInstance *term, const QString &infile) :
    TermTask(term, false),
    m_infile(infile)
{
    setup();
}

DownloadTask::DownloadTask(ServerInstance *server) :
    TermTask(server, true)
{
    setup();
}

void
DownloadTask::pushAck()
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_INPUT);
    m.addBytes(m_buf, 48);
    m.addNumber64(m_received);
    m_server->conn()->push(m.resultPtr(), m.length());
}

void
DownloadTask::pushCancel(int code)
{
    Tsq::ProtocolMarshaler m(TSQ_CANCEL_TASK);
    m.addBytes(m_buf, 48);
    m_server->conn()->push(m.resultPtr(), m.length());

    if (code)
        fail(strerror(code));
}

void
DownloadTask::cancel()
{
    closefd();

    // Write task cancel
    if (!finished())
        pushCancel(0);

    TermTask::cancel();
}

void
DownloadTask::openFile()
{
    pushAck();
}

void
DownloadTask::handleData(const char *buf, size_t len)
{
    if (len == 0) {
        // Finished, this should call finish() and closefd()
        finishFile();
        return;
    }

    if (!writeData(buf, len)) {
        pushCancel(errno);
        closefd();
        return;
    }

    setProgress(m_received += len, m_total);
    if (m_chunks < m_received / PAYLOADSIZE) {
        m_chunks = m_received / PAYLOADSIZE;
        pushAck();
    }
}

void
DownloadTask::start(TermManager *manager)
{
    if (TermTask::doStart(manager)) {
        // Write task start
        Tsq::ProtocolMarshaler m(TSQ_DOWNLOAD_FILE);
        m.addBytes(m_buf, 48);
        m.addNumberPair(PAYLOADSIZE, WINDOWSIZE);
        m.addBytes(m_infile.toStdString());

        m_server->conn()->push(m.resultPtr(), m.length());
    }
}

void
DownloadTask::handleStart(Tsq::ProtocolUnmarshaler *unm)
{
    m_mode = unm->parseNumber() & 0777;
    m_total = unm->parseNumber64();
}

void
DownloadTask::handleOutput(Tsq::ProtocolUnmarshaler *unm)
{
    if (finished())
        return;

    switch (unm->parseNumber()) {
    case Tsq::TaskRunning:
        handleData(unm->remainingBytes(), unm->remainingLength());
        break;
    case Tsq::TaskStarting:
        handleStart(unm);
        openFile();
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
DownloadTask::handleDisconnect()
{
    closefd();
}

bool
DownloadTask::clonable() const
{
    return finished() && m_target;
}

//
// Download to file
//
inline void
DownloadFileTask::setup()
{
    m_typeIcon = ICON_TASKTYPE_DOWNLOAD_FILE;
    m_sinkStr = TR_TASKOBJ2 + m_outfile;

    if (m_overwrite)
        m_config = Tsq::TaskOverwrite;
    else if (m_server->serverInfo()->downloadConfig() >= 0)
        m_config = m_server->serverInfo()->downloadConfig();
    else
        m_config = g_global->downloadConfig();
}

DownloadFileTask::DownloadFileTask(ServerInstance *server, const QString &infile,
                                   const QString &outfile, bool overwrite) :
    DownloadTask(server, infile),
    m_overwrite(overwrite),
    m_outfile(outfile)
{
    m_typeStr = TR_TASKTYPE1;
    m_sourceStr = TR_TASKOBJ2 + infile;

    setup();
}

DownloadFileTask::DownloadFileTask(TermInstance *term, const QString &contentId,
                                   const QString &outfile, bool overwrite) :
    DownloadTask(term, contentId),
    m_overwrite(overwrite),
    m_outfile(outfile)
{
    setup();
}

void
DownloadFileTask::closefd()
{
    if (m_fd != -1)
    {
        if (!succeeded()) {
            unlink(pr(m_outfile));
        }

        close(m_fd);
        m_fd = -1;
    }
}

DownloadFileTask::~DownloadFileTask()
{
    DownloadFileTask::closefd();
}

void
DownloadFileTask::openFile()
{
    std::string tmp = m_outfile.toStdString();

    // Overwrite scenarios
    if (osFileExists(tmp.c_str())) {
        switch (m_config) {
        case Tsq::TaskOverwrite:
            break;
        case Tsq::TaskAsk:
            setQuestion(Tsq::TaskOverwriteRenameQuestion, TR_ASK1);
            return;
        case Tsq::TaskRename:
            m_fd = osOpenRenamedFile(tmp, m_mode);
            m_outfile = QString::fromStdString(tmp);
            m_sinkStr = TR_TASKOBJ2 + m_outfile;
            emit taskChanged();
            goto out;
        default:
            pushCancel(EEXIST);
            return;
        }
    }

    m_fd = open(tmp.c_str(), O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOCTTY, m_mode);
out:
    if (m_fd < 0)
        pushCancel(errno);
    else
        pushAck();
}

void
DownloadFileTask::finishFile()
{
    finish();
    closefd();
    launch(m_outfile);
}

bool
DownloadFileTask::writeData(const char *buf, size_t len)
{
    size_t sent = 0;
    do {
        ssize_t rc = write(m_fd, buf, len - sent);
        if (rc < 0) {
            if (errno == EINTR)
                continue;

            return false;
        }
        sent += rc;
        buf += rc;
    } while (sent < len);

    return true;
}

void
DownloadFileTask::handleAnswer(int answer)
{
    if (!finished()) {
        clearQuestion();

        switch (m_config = answer) {
        case Tsq::TaskOverwrite:
        case Tsq::TaskRename:
            DownloadFileTask::openFile();
            break;
        default:
            pushCancel(0);
            TermTask::cancel();
            break;
        }
    }
}

TermTask *
DownloadFileTask::clone() const
{
    return new DownloadFileTask(m_server, m_infile, m_outfile, m_overwrite);
}

QString
DownloadFileTask::launchfile() const
{
    return succeeded() ? m_outfile : g_mtstr;
}

void
DownloadFileTask::getDragData(QMimeData *data) const
{
    if (succeeded()) {
        data->setText(m_outfile);
        data->setUrls(QList<QUrl>{ QUrl::fromLocalFile(m_outfile) });
    }
}

//
// Download to clipboard
//
CopyFileTask::CopyFileTask(ServerInstance *server, const QString &infile,
                           const QString &format) :
    DownloadTask(server, infile),
    m_format(format)
{
    m_typeStr = TR_TASKTYPE2;
    m_typeIcon = ICON_TASKTYPE_COPY_FILE;
    m_sourceStr = TR_TASKOBJ2 + infile;
    m_sinkStr = TR_TASKOBJ3;
}

CopyFileTask::CopyFileTask(TermInstance *term, const QString &contentId) :
    DownloadTask(term, contentId)
{
}

void
CopyFileTask::closefd()
{
    m_data.clear();
    m_data.squeeze();
}

void
CopyFileTask::finishFile()
{
    finishCopy(m_data, m_format);
    closefd();
}

bool
CopyFileTask::writeData(const char *buf, size_t len)
{
    m_data.append(buf, len);
    return true;
}

TermTask *
CopyFileTask::clone() const
{
    return new CopyFileTask(m_server, m_infile, m_format);
}

//
// Download to pipe
//
DownloadPipeTask::DownloadPipeTask(ServerInstance *server, ReaderConnection *reader) :
    DownloadTask(server),
    m_reader(reader),
    m_fd(reader->fd())
{
    reader->setParent(this);

    m_typeStr = TR_TASKTYPE3;
    m_typeIcon = ICON_TASKTYPE_DOWNLOAD_PIPE;
    m_sinkStr = TR_TASKOBJ4;

    m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_readNotifier, &QSocketNotifier::activated, this, &TermTask::cancel);
    m_writeNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Write, this);
    connect(m_writeNotifier, SIGNAL(activated(int)), SLOT(handleFile()));
    m_writeNotifier->setEnabled(false);
}

void
DownloadPipeTask::closefd()
{
    if (m_fd != -1) {
        m_readNotifier->setEnabled(false);
        m_writeNotifier->setEnabled(false);
        close(m_fd);
        m_fd = -1;
    }
}

void
DownloadPipeTask::finishFile()
{
    closefd();
    finish();
}

bool
DownloadPipeTask::writeData(const char *, size_t)
{
    // This function is unused by our handleData
    return false;
}

void
DownloadPipeTask::handleData(const char *buf, size_t len)
{
    ssize_t rc;

    if (!m_outdata.empty() || len == 0)
        goto push;

    // Try a quick write
    rc = write(m_fd, buf, len);
    if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            goto push;

        // Fail
        pushCancel(errno);
        closefd();
        return;
    }

    if (m_chunks < m_received / PAYLOADSIZE) {
        m_chunks = m_received / PAYLOADSIZE;
        pushAck();
    }
    queueTaskChange();

    if (rc == len)
        return;

    buf += rc;
    len -= rc;
push:
    m_outdata.emplace(buf, len);
    m_writeNotifier->setEnabled(true);
}

void
DownloadPipeTask::handleFile()
{
    if (m_outdata.empty()) {
        m_writeNotifier->setEnabled(false);
        return;
    }

    QByteArray &data = m_outdata.front();
    size_t len = data.size();
    ssize_t rc;

    if (len == 0) {
        finishFile();
        return;
    }
    else if ((rc = write(m_fd, data.data(), len)) == len) {
        m_outdata.pop();
    }
    else if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            return;

        // Fail
        pushCancel(errno);
        closefd();
        return;
    }
    else {
        data.remove(0, rc);
    }

    if (m_chunks < m_received / PAYLOADSIZE) {
        m_chunks = m_received / PAYLOADSIZE;
        pushAck();
    }
    queueTaskChange();
}

void
DownloadPipeTask::start(TermManager *manager)
{
    if (TermTask::doStart(manager)) {
        // Write task start
        Tsq::ProtocolMarshaler m(TSQ_DOWNLOAD_PIPE);
        m.addBytes(m_buf, 48);
        m.addNumberPair(PAYLOADSIZE, WINDOWSIZE);
        m.addNumber(0644);

        m_server->conn()->push(m.resultPtr(), m.length());
    }
}

void
DownloadPipeTask::handleStart(Tsq::ProtocolUnmarshaler *unm)
{
    std::string name = unm->parseString();
    m_reader->pushPipeMessage(0, name);
    m_sourceStr = TR_TASKOBJ5 + QString::fromStdString(name);
    emit taskChanged();
}

bool
DownloadPipeTask::clonable() const
{
    return false;
}
