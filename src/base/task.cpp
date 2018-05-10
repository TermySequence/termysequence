// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "task.h"
#include "term.h"
#include "server.h"
#include "conn.h"
#include "listener.h"
#include "taskmodel.h"
#include "manager.h"
#include "infoanim.h"
#include "settings/global.h"
#include "lib/utf8.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>

#define TR_TASKOBJ1 TL("task-object", "Terminal") + ':'
#define TR_TASKOBJ2 TL("task-object", "Server") + ':'
#define TR_TASKSTAT1 TL("task-status", "Running")
#define TR_TASKSTAT2 TL("task-status", "In Progress")
#define TR_TASKSTAT3 TL("task-status", "Waiting for user input")
#define TR_TASKSTAT4 TL("task-status", "Disconnected")
#define TR_TASKSTAT5 TL("task-status", "Completed")
#define TR_TASKSTAT6 TL("task-status", "Canceled")
#define TR_TASKSTAT7 TL("task-status", "Error")
#define TR_ERROR1 TL("error", "Server is not connected")
#define TR_ERROR2 TL("error", "Copy failed: text is not valid UTF-8")
#define TR_ERROR3 TL("error", "Copy failed: data format not recognized")

TermTask::TermTask(bool longRunning) :
    QObject(g_listener),
    m_longRunning(longRunning),
    m_noTarget(true),
    m_taskIdStr(QString::fromLatin1(m_taskId.str().c_str())),
    m_target(nullptr),
    m_server(nullptr),
    m_term(nullptr)
{
}

TermTask::TermTask(TermInstance *term, bool longRunning) :
    QObject(g_listener),
    m_longRunning(longRunning),
    m_serverId(term->server()->id()),
    m_termId(term->id()),
    m_taskIdStr(QString::fromLatin1(m_taskId.str().c_str())),
    m_target(term),
    m_server(term->server()),
    m_term(term)
{
    m_toStr = m_fromStr = TR_TASKOBJ1 + term->title();
}

TermTask::TermTask(ServerInstance *server, bool longRunning) :
    QObject(g_listener),
    m_longRunning(longRunning),
    m_serverId(server ? server->id() : Tsq::Uuid()),
    m_taskIdStr(QString::fromLatin1(m_taskId.str().c_str())),
    m_target(server),
    m_server(server),
    m_term(nullptr)
{
}

inline void
TermTask::setTargetStr()
{
    if (m_toStr.isEmpty())
        m_toStr = TR_TASKOBJ2 + m_server->shortname();
    if (m_fromStr.isEmpty())
        m_fromStr = TR_TASKOBJ2 + m_server->shortname();
}

void
TermTask::setStatus(const QString &msg, int progress)
{
    if (progress)
        m_progress = progress;

    m_statusStr = msg;
}

void
TermTask::setProgress(size_t numer, size_t denom)
{
    int progress = denom ? (100 * numer / denom) : 100;

    if (m_progress != progress) {
        m_progress = progress;
        m_statusStr = TR_TASKSTAT2 + L(": %1%").arg(progress);
        emit taskChanged();
    }
}

void
TermTask::timerEvent(QTimerEvent *)
{
    killTimer(m_timerId);
    m_timerId = 0;
    emit taskChanged();
}

void
TermTask::setQuestion(Tsq::TaskQuestion type, const QString &msg)
{
    if (!m_finished) {
        m_questioning = true;
        m_questionType = type;
        m_questionStr = msg;
        m_statusStr = TR_TASKSTAT3;
        emit taskQuestion();
        emit taskChanged();
    }
}

void
TermTask::clearQuestion()
{
    m_questioning = false;

    if (!m_finished)
        m_statusStr = m_longRunning ?
            TR_TASKSTAT2 + L(": %1%").arg(m_progress) :
            TR_TASKSTAT1;

    emit taskQuestion();
    emit taskChanged();
}

void
TermTask::failStart(TermManager *manager, const QString &msg)
{
    m_started = m_finished = true;
    m_startedDate = m_finishedDate = QDateTime::currentDateTime();
    m_startedStr = m_finishedStr = m_startedDate.toString();
    m_statusStr = TR_TASKSTAT7 + ':' + msg;
    if (m_target)
        setTargetStr();

    m_noStatusPopup = false;

    m_animation = new InfoAnimation(this, (intptr_t)this, 2);
    g_listener->taskmodel()->addTask(this, manager);
    m_animation->startColorName(ErrorFg);
}

bool
TermTask::doStart(TermManager *manager)
{
    if ((!m_target && !m_noTarget) || (m_server && !m_server->conn())) {
        failStart(manager, TR_ERROR1);
        return false;
    }

    m_started = true;
    m_startedDate = QDateTime::currentDateTime();
    m_startedStr = m_startedDate.toString();
    m_statusStr = m_longRunning ? TR_TASKSTAT1 : TR_TASKSTAT2 + A(": 0%");
    m_progress = m_longRunning ? -1 : 0;

    if (m_target) {
        setTargetStr();
        connect(m_server, SIGNAL(connectionChanged()), SLOT(handleTargetDisconnected()));
        connect(m_target, SIGNAL(destroyed()), SLOT(handleTargetDestroyed()));
        m_mocTask = connect(m_target, SIGNAL(throttleChanged(bool)), SLOT(handleThrottle(bool)));
        if (m_target->throttled())
            handleThrottle(true);
    }

    m_animation = new InfoAnimation(this, (intptr_t)this, 2);
    g_listener->taskmodel()->addTask(this, manager);
    m_animation->startColorName(StartFg);
    return true;
}

void
TermTask::stop(const QString &msg, ColorName color, bool destroyed)
{
    if (m_timerId != 0)
        killTimer(m_timerId);

    m_progress = -2;
    m_finished = true;
    m_finishedDate = QDateTime::currentDateTime();
    m_finishedStr = m_finishedDate.toString();
    m_statusStr = msg;
    m_finishColor = color;

    if (m_questioning)
        clearQuestion();
    else
        emit taskChanged();

    if (!destroyed) {
        disconnect(m_mocTask);
        m_animation->startColorName(color);
    }
}

void
TermTask::handleTargetDisconnected()
{
    if (!m_finished) {
        handleDisconnect();
        stop(TR_TASKSTAT4, ErrorFg);
    }
}

void
TermTask::handleTargetDestroyed()
{
    if (!m_finished) {
        handleDisconnect();
        stop(TR_TASKSTAT4, ErrorFg, true);
    }

    m_target = nullptr;
    m_server = nullptr;
    m_term = nullptr;
}

void
TermTask::setDialog(QObject *dialog) {
    if (m_dialog)
        m_dialog->disconnect(this);

    m_dialog = dialog;
    connect(dialog, &QObject::destroyed, this, [this]{ m_dialog = nullptr; });
}

void
TermTask::finish()
{
    m_succeeded = true;
    stop(TR_TASKSTAT5, FinishFg);
}

void
TermTask::cancel()
{
    if (!m_finished) {
        if (m_questioning)
            m_questionCancel = true;

        stop(TR_TASKSTAT6, CancelFg);
    }
}

void
TermTask::fail(const QString &msg)
{
    stop(TR_TASKSTAT7 + ':' + msg, ErrorFg);
}

void
TermTask::launch(const QString &file)
{
    emit ready(file);
    auto *manager = g_listener->activeManager();
    int action;

    if (!manager)
        action = TaskActNothing;
    else if (inherits("MountTask"))
        action = g_global->mountAction();
    else
        action = g_global->downloadAction();

    switch (action) {
    case TaskActFile:
        manager->actionOpenTaskFile(g_mtstr, m_taskIdStr);
        break;
    case TaskActFileDesk:
        manager->actionOpenTaskFile(g_str_DESKTOP_LAUNCH, m_taskIdStr);
        break;
    case TaskActDir:
        manager->actionOpenTaskDirectory(g_mtstr, m_taskIdStr);
        break;
    case TaskActDirDesk:
        manager->actionOpenTaskDirectory(g_str_DESKTOP_LAUNCH, m_taskIdStr);
        break;
    case TaskActTerm:
        manager->actionOpenTaskTerminal(g_mtstr, m_taskIdStr);
        break;
    }
}

QString
TermTask::launchfile() const
{
    return g_mtstr;
}

void
TermTask::getDragData(QMimeData *) const
{}

bool
TermTask::clonable() const
{
    return false;
}

TermTask *
TermTask::clone() const
{
    return nullptr;
}

void
TermTask::handleOutput(Tsq::ProtocolUnmarshaler *)
{}

void
TermTask::handleQuestion(int)
{}

void
TermTask::handleAnswer(int)
{}

void
TermTask::handleThrottle(bool)
{}

void
TermTask::handleDisconnect()
{}

static inline void
reportClipboardCopy(int size, int type)
{
    auto *manager = g_listener->activeManager();
    if (manager)
        manager->reportClipboardCopy(size, type);
}

bool
TermTask::copyAsText(const QByteArray &data)
{
    const char *ptr = data.data();
    if (utf8::is_valid(ptr, ptr + data.size())) {
        QString text = data;
        QApplication::clipboard()->setText(text);
        reportClipboardCopy(text.size(), CopiedChars);
        finish();
        return true;
    }
    return false;
}

bool
TermTask::copyAsImage(const QByteArray &data)
{
    QImage image;
    if (image.loadFromData(data)) {
        QApplication::clipboard()->setImage(image);
        reportClipboardCopy(0, CopiedImage);
        finish();
        return true;
    }
    return false;
}

void
TermTask::finishCopy(const QByteArray &data, const QString &format)
{
    if (format.isEmpty()) {
        if (!copyAsText(data) && !copyAsImage(data))
            fail(TR_ERROR3);
    }
    else if (format == A("0")) {
        if (!copyAsText(data))
            fail(TR_ERROR2);
    }
    else if (format == A("1")) {
        if (!copyAsImage(data))
            fail(TR_ERROR3);
    }
    else {
        QMimeData *md = new QMimeData;
        md->setData(format, data);
        QApplication::clipboard()->setMimeData(md);
        reportClipboardCopy(data.size(), CopiedBytes);
        finish();
    }
}
