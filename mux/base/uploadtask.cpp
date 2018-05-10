// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "uploadtask.h"
#include "listener.h"
#include "exception.h"
#include "os/fd.h"
#include "os/dir.h"
#include "os/logging.h"
#include "lib/wire.h"
#include "lib/protocol.h"

#include <cstdio>
#include <unistd.h>
#include <fcntl.h>

//
// Upload to file
//
FileUpload::FileUpload(Tsq::ProtocolUnmarshaler *unm) :
    TaskBase("upload", unm, TaskBaseExclusive|ThreadBaseFd)
{
    m_chunkSize = unm->parseNumber();
    m_mode = unm->parseNumber() & 0777;
    m_config = unm->parseNumber();
    m_targetName = m_fileName = unm->parseString();
}

FileUpload::FileUpload(Tsq::ProtocolUnmarshaler *unm, unsigned flags) :
    TaskBase("pipeout", unm, flags|ThreadBaseFd)
{
    m_chunkSize = unm->parseNumber();
    m_mode = unm->parseNumber() & 0777;
    m_timeout = -1;
}

/*
 * This thread
 */
void
FileUpload::reportError(int errtype, int errnum)
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumber(Tsq::TaskError);
    m.addNumber64(m_received);
    m.addNumber(errtype);
    m.addString(strerror(errnum));
    g_listener->forwardToClient(m_clientId, m.result());
}

void
FileUpload::reportStatus(Tsq::TaskStatus status, const char *buf, unsigned len)
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumber(status);
    m.addNumber64(m_received);
    m.addBytes(buf, len);
    g_listener->forwardToClient(m_clientId, m.result());
}

inline void
FileUpload::closefd()
{
    if (m_fd != -1) {
        if (!m_succeeded) {
            unlink(m_fileName.c_str());
        }

        close(m_fd);
        m_fd = -1;
    }
}

bool
FileUpload::openfd()
{
    // Overwrite scenarios
    if (osFileExists(m_fileName.c_str())) {
        switch (m_config & 0xffff) {
        case Tsq::TaskOverwrite:
            break;
        case Tsq::TaskAsk:
            reportQuestion(Tsq::TaskOverwriteRenameQuestion);
            return true;
        case Tsq::TaskRename:
            setfd(osOpenRenamedFile(m_fileName, m_mode));
            reportStatus(Tsq::TaskStarting, m_fileName.data(), m_fileName.size());
            goto out;
        default:
            reportError(Tsq::UploadTaskErrorFileExists, EEXIST);
            LOGDBG("Upload %p: failed, target file exists\n", this);
            return false;
        }
    }
    else if (m_config & 0x80000000) {
        osMkpath(m_fileName, m_mode);
    }

    setfd(open(m_fileName.c_str(), O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOCTTY, m_mode));
out:
    if (m_fd >= 0) {
        reportStatus(Tsq::TaskAcking, nullptr, 0);
        LOGDBG("Upload %p: running\n", this);
        return true;
    }

    // Fail
    reportError(Tsq::UploadTaskErrorOpenFailed, errno);
    LOGNOT("Upload %p: failed to open '%s' for writing: %m\n", this, m_fileName.c_str());
    return false;
}

bool
FileUpload::handleFd()
{
    if (m_outdata.empty()) {
        enablefd(false);
        return true;
    }

    std::string *data = m_outdata.front();
    size_t len = data->size();
    ssize_t rc;

    if (len == 0) {
        // Finished uploading
        m_succeeded = true;
        reportStatus(Tsq::TaskFinished, nullptr, 0);
        LOGDBG("Upload %p: finished\n", this);
        return false;
    }
    else if ((rc = write(m_fd, data->data(), len)) == len) {
        {
            Lock lock(this);
            m_incomingData.erase(data);
        }
        m_outdata.pop();
        delete data;
    }
    else if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            return true;

        // Fail
        reportError(Tsq::UploadTaskErrorWriteFailed, errno);
        LOGNOT("Upload %p: write failed: %m\n", this);
        return false;
    }
    else {
        data->erase(0, rc);
    }

    m_received += rc;
    if (m_chunks < m_received / m_chunkSize) {
        m_chunks = m_received / m_chunkSize;
        reportStatus(Tsq::TaskAcking, nullptr, 0);
    }

    return true;
}

bool
FileUpload::handleData(std::string *data)
{
    size_t len = data->size();
    ssize_t rc;

    if (!m_outdata.empty() || len == 0)
        goto push;

    // Try a quick write
    rc = write(m_fd, data->data(), len);
    if (rc < 0) {
        if (errno == EAGAIN || errno == EINTR)
            goto push;

        // Fail
        reportError(Tsq::UploadTaskErrorWriteFailed, errno);
        LOGNOT("Upload %p: write failed: %m\n", this);
        return false;
    }

    m_received += rc;
    if (m_chunks < m_received / m_chunkSize) {
        m_chunks = m_received / m_chunkSize;
        reportStatus(Tsq::TaskAcking, nullptr, 0);
    }

    if (rc == len) {
        {
            Lock lock(this);
            m_incomingData.erase(data);
        }
        delete data;
        return true;
    }

    data->erase(0, rc);
push:
    // Keep data in incoming pool until written
    m_outdata.push(data);
    m_fds[1].events = POLLOUT;
    return true;
}

bool
FileUpload::handleAnswer(int answer)
{
    if (!m_questioning) {
        LOGNOT("Upload %p: unexpected answer\n", this);
        return false;
    }

    m_questioning = false;
    LOGDBG("Upload %p: question answered: %d\n", this, answer);

    switch (m_config = answer) {
    case Tsq::TaskOverwrite:
    case Tsq::TaskRename:
        return openfd();
    default:
        reportError(Tsq::TaskErrorCanceled, ECANCELED);
        return false;
    }
}

bool
FileUpload::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case TaskClose:
        LOGDBG("Upload %p: canceled (code %td)\n", this, item.value);
        return false;
    case TaskInput:
        return handleData((std::string *)item.value);
    case TaskAnswer:
        return handleAnswer(item.value);
    default:
        break;
    }

    return true;
}

bool
FileUpload::handleIdle()
{
    LOGNOT("Upload %p: idle timeout exceeded\n", this);
    reportError(Tsq::TaskErrorTimedOut, ETIMEDOUT);
    return false;
}

void
FileUpload::threadMain()
{
    enablefd(false);

    try {
        if (openfd())
            runDescriptorLoop();
    }
    catch (const std::exception &e) {
        LOGERR("Upload %p: caught exception: %s\n", this, e.what());
    }

    closefd();
    g_listener->sendWork(ListenerRemoveTask, this);
}

//
// Upload to pipe
//
PipeUpload::PipeUpload(Tsq::ProtocolUnmarshaler *unm) :
    FileUpload(unm, 0)
{
}

PipeUpload::~PipeUpload()
{
    unlink(m_fileName.c_str());
}

bool
PipeUpload::openfd()
{
    try {
        setfd(osCreateNamedPipe(false, m_mode, m_fileName));
    }
    catch (const std::exception &e) {
        // Fail
        reportError(Tsq::UploadTaskErrorOpenFailed, ENOENT);
        LOGNOT("PipeOut %p: failed to create fifo: %s\n", this, e.what());
        return false;
    }

    reportStatus(Tsq::TaskStarting, m_fileName.data(), m_fileName.size());
    reportStatus(Tsq::TaskAcking, nullptr, 0);
    LOGDBG("PipeOut %p: running\n", this);
    return true;
}

bool
PipeUpload::handleAnswer(int)
{
    return false;
}

bool
PipeUpload::handleIdle()
{
    return true;
}
