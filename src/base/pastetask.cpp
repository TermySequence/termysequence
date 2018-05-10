// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/icons.h"
#include "pastetask.h"
#include "term.h"
#include "conn.h"
#include "listener.h"
#include "os/fd.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/sequences.h"

#include <QSocketNotifier>
#include <cerrno>
#include <exception>
#include <unistd.h>

#define TR_TASKOBJ1 TL("task-object", "This computer")
#define TR_TASKOBJ2 TL("task-object", "File") + ':'
#define TR_TASKOBJ3 TL("task-object", "Terminal input stream")
#define TR_TASKOBJ4 TL("task-object", "This host")
#define TR_TASKOBJ5 TL("task-object", "Pasted data: %1 bytes")
#define TR_TASKTYPE1 TL("task-type", "Paste From File")
#define TR_TASKTYPE2 TL("task-type", "Paste")

#define BUFSIZE (4 * TERM_PAYLOADSIZE)
#define HEADERSIZE 40
#define PAYLOADSIZE (BUFSIZE - HEADERSIZE)

//
// Paste from file
//
PasteFileTask::PasteFileTask(TermInstance *term, const QString &fileName,
                             TermTask *prereq) :
    TermTask(term, false),
    m_fd(-1),
    m_bracketed(m_term->flags() & Tsq::BracketedPasteMode),
    m_fileName(fileName),
    m_prereq(prereq)
{
    m_typeStr = TR_TASKTYPE1;
    m_typeIcon = ICON_TASKTYPE_PASTE_FILE;
    m_fromStr = TR_TASKOBJ1;
    m_sourceStr = TR_TASKOBJ2 + fileName;
    m_sinkStr = TR_TASKOBJ3;

    uint32_t command = htole32(TSQ_INPUT);

    m_buf = new char[BUFSIZE];
    memcpy(m_buf, &command, 4);
    memcpy(m_buf + 8, term->id().buf, 16);
    memcpy(m_buf + 24, g_listener->id().buf, 16);
    m_ptr = m_buf + HEADERSIZE;
}

void
PasteFileTask::pushBytes(size_t len)
{
    uint32_t length = htole32(HEADERSIZE - 8 + len);
    memcpy(m_buf + 4, &length, 4);

    m_server->conn()->push(m_buf, HEADERSIZE + len);
    setProgress(m_sent += len, m_total);
}

void
PasteFileTask::closefd()
{
    if (m_fd != -1)
    {
        // Write BPM end
        if (m_bracketed && !finished()) {
            memcpy(m_ptr, TSQ_BPM_END, TSQ_BPM_END_LEN);
            pushBytes(TSQ_BPM_END_LEN);
        }

        m_notifier->setEnabled(false);
        close(m_fd);
        m_fd = -1;
    }
}

PasteFileTask::~PasteFileTask()
{
    m_bracketed = false;
    closefd();
    delete [] m_buf;
}

void
PasteFileTask::reallyStart()
{
    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, SIGNAL(activated(int)), SLOT(handleFile(int)));

    // Write BPM start
    if (m_bracketed) {
        m_total += TSQ_BPM_START_LEN + TSQ_BPM_END_LEN;
        memcpy(m_ptr, TSQ_BPM_START, TSQ_BPM_START_LEN);
        pushBytes(TSQ_BPM_START_LEN);
    }
}

void
PasteFileTask::handlePrereq()
{
    if (m_prereq->finished()) {
        m_prereq->disconnect(this);
        if (m_prereq->succeeded())
            reallyStart();
        else
            cancel();
    }
}

void
PasteFileTask::start(TermManager *manager)
{
    try {
        m_fd = osOpenFile(pr(m_fileName), &m_total, nullptr);
    } catch (const std::exception &e) {
        failStart(manager, e.what());
        return;
    }

    if (TermTask::doStart(manager)) {
        if (!m_prereq) {
            reallyStart();
        } else if (m_prereq->finished()) {
            handlePrereq();
        } else {
            connect(m_prereq, &QObject::destroyed, this, &PasteFileTask::cancel);
            connect(m_prereq, SIGNAL(taskChanged()), SLOT(handlePrereq()));
        }
    } else {
        closefd();
    }
}

void
PasteFileTask::handleFile(int fd)
{
    ssize_t rc;

    rc = read(fd, m_ptr, PAYLOADSIZE);
    if (rc == 0) {
        closefd();
        finish();
    }
    else if (rc < 0) {
        fail(strerror(errno));
        closefd();
    }
    else {
        pushBytes(rc);
    }
}

void
PasteFileTask::cancel()
{
    // Order matters
    TermTask::cancel();
    closefd();
}

void
PasteFileTask::handleDisconnect()
{
    closefd();
}

void
PasteFileTask::handleThrottle(bool throttled)
{
    m_notifier->setEnabled(!throttled);
}

bool
PasteFileTask::clonable() const
{
    return finished() && m_target;
}

TermTask *
PasteFileTask::clone() const
{
    return new PasteFileTask(m_term, m_fileName);
}

//
// Paste from memory
//
PasteBytesTask::PasteBytesTask(TermInstance *term, const QByteArray &data) :
    TermTask(term, false),
    m_data(data),
    m_timerId(0)
{
    m_ptr = m_data.data();
    m_total = m_data.size();

    m_typeStr = TR_TASKTYPE2;
    m_typeIcon = ICON_TASKTYPE_PASTE_BYTES;
    m_fromStr = TR_TASKOBJ4;
    m_sourceStr = TR_TASKOBJ5.arg(m_total);
    m_sinkStr = TR_TASKOBJ3;

    uint32_t command = htole32(TSQ_INPUT);

    memcpy(m_buf, &command, 4);
    memcpy(m_buf + 8, term->id().buf, 16);
    memcpy(m_buf + 24, g_listener->id().buf, 16);
}

void
PasteBytesTask::setTimer(bool enabled)
{
    if (!enabled && m_timerId)
    {
        killTimer(m_timerId);
        m_timerId = 0;
    }
    else if (enabled && m_timerId == 0)
    {
        m_timerId = startTimer(0);
    }
}

void
PasteBytesTask::pushBytes(size_t len)
{
    uint32_t length = htole32(HEADERSIZE - 8 + len);
    memcpy(m_buf + 4, &length, 4);

    m_server->conn()->send(m_buf, HEADERSIZE);
    m_server->conn()->push(m_ptr, len);
    setProgress(m_sent += len, m_total);
    m_ptr += len;
}

void
PasteBytesTask::start(TermManager *manager)
{
    // Order matters, due to throttle handler
    setTimer(true);

    if (!TermTask::doStart(manager))
        setTimer(false);
}

void
PasteBytesTask::timerEvent(QTimerEvent *)
{
    size_t rem = m_total - m_sent;

    if (rem == 0) {
        setTimer(false);
        finish();
    } else {
        pushBytes(rem < PAYLOADSIZE ? rem : PAYLOADSIZE);
    }
}

void
PasteBytesTask::cancel()
{
    setTimer(false);
    TermTask::cancel();
}

void
PasteBytesTask::handleDisconnect()
{
    setTimer(false);
}

void
PasteBytesTask::handleThrottle(bool throttled)
{
    setTimer(!throttled);
}

bool
PasteBytesTask::clonable() const
{
    return finished() && m_target;
}

TermTask *
PasteBytesTask::clone() const
{
    return new PasteBytesTask(m_term, m_data);
}
