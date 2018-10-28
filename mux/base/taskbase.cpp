// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "taskbase.h"
#include "listener.h"
#include "exception.h"
#include "lib/wire.h"
#include "lib/protocol.h"
#include "config.h"

TaskBase::TaskBase(const char *name, Tsq::ProtocolUnmarshaler *unm, unsigned flags) :
    ThreadBase(name, flags),
    m_questioning(false),
    m_throttled(false),
    m_throttlable(flags & TaskBaseThrottlable),
    m_exclusive(flags & TaskBaseExclusive)
{
    m_clientId = unm->parseUuid();
    m_taskId = unm->parseUuid();

    m_timeout = m_exclusive ? FILETASK_IDLE_TIME : FILETASK_IDLE_TIME * 10;
}

TaskBase::~TaskBase()
{
    forDeleteAll(m_incomingData);
}

/*
 * Other threads
 */
void
TaskBase::sendInput(std::string &data)
{
    std::string *copy = new std::string(std::move(data));

    FlexLock lock(this);

    m_incomingData.insert(copy);
    sendWorkAndUnlock(lock, TaskInput, copy);
}

void
TaskBase::pause(const Tsq::Uuid &hopId)
{
    FlexLock lock(this);

    bool wasEmpty = m_throttles.empty();
    m_throttles.emplace(hopId);

    if (wasEmpty)
        sendWorkAndUnlock(lock, TaskPause, 0);
}

void
TaskBase::resume(const Tsq::Uuid &hopId)
{
    FlexLock lock(this);

    m_throttles.erase(hopId);

    if (m_throttles.empty())
        sendWorkAndUnlock(lock, TaskResume, 0);
}

void
TaskBase::reportError(int errtype, const char *errstr)
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_OUTPUT);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumberPair(Tsq::TaskError, errtype);
    m.addString(errstr);
    g_listener->forwardToClient(m_clientId, m.result());
}

void
TaskBase::reportDuplicateTask()
{
    reportError(Tsq::TaskErrorTargetInUse,
                "another task is using the target file or address");
}

/*
 * This thread
 */
bool
TaskBase::throttledOutput(std::string &buf)
{
    switch (g_listener->forwardToClient(m_clientId, buf)) {
    case 0: {
        Lock lock(this);
        m_throttles.emplace(g_listener->id());
        return false;
    }
    case -1:
        throw ErrnoException(ENOTCONN);
    default:
        return true;
    }
}

void
TaskBase::reportQuestion(Tsq::TaskQuestion question)
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_QUESTION);
    m.addUuidPair(m_clientId, g_listener->id());
    m.addUuid(m_taskId);
    m.addNumber(question);
    g_listener->forwardToClient(m_clientId, m.result());

    m_questioning = true;
}
