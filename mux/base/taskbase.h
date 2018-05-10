// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "threadbase.h"
#include "lib/uuid.h"
#include "lib/enums.h"

#include <unordered_set>

namespace Tsq { class ProtocolUnmarshaler; }

class TaskBase: public ThreadBase
{
protected:
    Tsq::Uuid m_clientId;
    Tsq::Uuid m_taskId;

    // locked
    std::unordered_set<std::string *> m_incomingData;
    std::unordered_set<Tsq::Uuid> m_throttles;

    std::string m_targetName;
    unsigned m_config;

    bool m_questioning;
    bool m_throttled;

    void reportError(int errtype, const char *errstr);

private:
    bool m_throttlable;
    bool m_exclusive;

public:
    TaskBase(const char *name, Tsq::ProtocolUnmarshaler *unm, unsigned flags);
    ~TaskBase();

    inline const Tsq::Uuid& clientId() const { return m_clientId; }
    inline const Tsq::Uuid& taskId() const { return m_taskId; }

    inline const std::string& targetName() const { return m_targetName; }
    inline bool throttlable() const { return m_throttlable; }
    inline bool exclusive() const { return m_exclusive; }

    void sendInput(std::string &data);
    void pause(const Tsq::Uuid &hopId);
    void resume(const Tsq::Uuid &hopId);

protected:
    bool throttledOutput(std::string &data);
    void reportQuestion(Tsq::TaskQuestion question);

public:
    void reportDuplicateTask();
};

enum TaskWork {
    TaskClose,
    TaskInput,
    TaskAnswer,
    TaskPause,
    TaskResume,
    TaskPrivate,
    TaskProcessExited, // ProcessExited common value
};
