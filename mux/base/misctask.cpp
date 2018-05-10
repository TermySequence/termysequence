// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "misctask.h"
#include "listener.h"
#include "exception.h"
#include "os/fd.h"
#include "os/logging.h"
#include "lib/wire.h"
#include "lib/protocol.h"

#include <cstdio>
#include <unistd.h>

FileMisc::FileMisc(Tsq::ProtocolUnmarshaler *unm, bool rename) :
    TaskBase("filemisc", unm, TaskBaseExclusive),
    m_rename(rename)
{
    m_config = unm->parseNumber();
    m_targetName = unm->parseString();

    if (rename)
        m_destName = unm->parseString();
}

/*
 * This thread
 */
void
FileMisc::reportStatus(Tsq::TaskStatus status)
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumber(status);
    g_listener->forwardToClient(m_clientId, m.result());
}

bool
FileMisc::runRemove()
{
    bool exists, isdir;
    exists = osFileExists(m_targetName.c_str(), &isdir);

    if (!exists) {
        reportError(Tsq::DeleteTaskErrorFileNotFound, strerror(errno));
        LOGDBG("FileMisc %p: failed to stat '%s': %m\n", this, m_targetName.c_str());
        return false;
    }
    else if (isdir && (m_config == Tsq::TaskAsk || m_config == Tsq::TaskAskRecurse)) {
        reportQuestion(Tsq::TaskRecurseQuestion);
        LOGDBG("FileMisc %p: requesting recursive remove confirmation\n", this);
        return true;
    }
    else if (m_config == Tsq::TaskAsk) {
        reportQuestion(Tsq::TaskRemoveQuestion);
        LOGDBG("FileMisc %p: requesting simple remove confirmation\n", this);
        return true;
    }
    else if (remove(m_targetName.c_str()) == 0 || (errno == ENOTEMPTY &&
             m_config == Tsq::TaskOverwrite && !osRecursiveDelete(m_targetName))) {
        reportStatus(Tsq::TaskFinished);
        LOGDBG("FileMisc %p: finished\n", this);
        return false;
    }
    else {
        reportError(Tsq::DeleteTaskErrorRemoveFailed, strerror(errno));
        LOGDBG("FileMisc %p: failed to remove '%s': %m\n", this, m_targetName.c_str());
        return false;
    }
}

bool
FileMisc::runRename()
{
    if (m_destName.empty() || m_destName.front() != '/') {
        reportError(Tsq::RenameTaskErrorInvalidName, strerror(EINVAL));
        LOGDBG("FileMisc %p: failed, invalid destination\n", this);
        return false;
    }

    // Overwrite scenarios
    if (osFileExists(m_destName.c_str())) {
        switch (m_config) {
        case Tsq::TaskOverwrite:
            break;
        case Tsq::TaskAsk:
            reportQuestion(Tsq::TaskOverwriteQuestion);
            LOGDBG("FileMisc %p: requesting overwrite confirmation\n", this);
            return true;
        default:
            reportError(Tsq::RenameTaskErrorFileExists, strerror(EEXIST));
            LOGDBG("FileMisc %p: failed, target file exists\n", this);
            return false;
        }
    }

    if (rename(m_targetName.c_str(), m_destName.c_str()) == 0) {
        reportStatus(Tsq::TaskFinished);
        LOGDBG("FileMisc %p: finished\n", this);
    } else {
        reportError(Tsq::RenameTaskErrorRenameFailed, strerror(errno));
        LOGDBG("FileMisc %p: failed to rename '%s' to '%s': %m\n", this,
               m_targetName.c_str(), m_destName.c_str());
    }

    return false;
}

bool
FileMisc::handleAnswer(int answer)
{
    if (!m_questioning) {
        LOGNOT("FileMisc %p: unexpected answer\n", this);
        return false;
    }

    m_questioning = false;
    LOGDBG("FileMisc %p: question answered: %d\n", this, answer);

    if (answer == Tsq::TaskOverwrite) {
        m_config = Tsq::TaskOverwrite;
        sendWork(TaskPrivate, 0);
        return true;
    } else {
        reportError(Tsq::TaskErrorCanceled, strerror(ECANCELED));
        return false;
    }
}

bool
FileMisc::handleWork(const WorkItem &item)
{
    switch (item.type) {
    case TaskClose:
        LOGDBG("FileMisc %p: canceled (code %td)\n", this, item.value);
        return false;
    case TaskInput:
        // No data for misc tasks
        return false;
    case TaskAnswer:
        return handleAnswer(item.value);
    case TaskPrivate:
        return m_rename ? runRename() : runRemove();
    default:
        break;
    }

    return true;
}

bool
FileMisc::handleIdle()
{
    LOGNOT("FileMisc %p: idle timeout exceeded\n", this);
    reportError(Tsq::TaskErrorTimedOut, strerror(ETIMEDOUT));
    return false;
}

void
FileMisc::threadMain()
{
    sendWork(TaskPrivate, 0);

    try {
        runDescriptorLoopWithoutFd();
    }
    catch (const std::exception &e) {
        LOGERR("FileMisc %p: caught exception: %s\n", this, e.what());
    }

    g_listener->sendWork(ListenerRemoveTask, this);
}
