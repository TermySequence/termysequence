// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attr.h"
#include "app/icons.h"
#include "app/logging.h"
#include "app/messagebox.h"
#include "app/reaper.h"
#include "runtask.h"
#include "conn.h"
#include "listener.h"
#include "manager.h"
#include "connectstatus.h"
#include "settings/launcher.h"
#include "os/fd.h"
#include "os/dir.h"
#include "os/process.h"
#include "lib/exception.h"
#include "lib/protocol.h"
#include "lib/wire.h"
#include "lib/sequences.h"
#include "lib/attrstr.h"
#include "config.h"

#include <QSocketNotifier>
#include <QUrl>
#include <QMimeData>

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#define TR_ASK1 TL("question", "File exists. Overwrite?")
#define TR_TASKOBJ1 TL("task-object", "N/A")
#define TR_TASKOBJ2 TL("task-object", "Command") + ':'
#define TR_TASKOBJ3 TL("task-object", "Launcher") + ':'
#define TR_TASKOBJ4 TL("task-object", "PID") + ':'
#define TR_TASKTYPE1 TL("task-type", "Remote Command")
#define TR_TASKTYPE2 TL("task-type", "Local Command")
#define TR_TEXT1 TL("window-text", "Choose input file for command")
#define TR_TEXT2 TL("window-text", "Choose output file for command")

#define BUFSIZE (16 * TERM_PAYLOADSIZE)
#define HEADERSIZE 60
#define PAYLOADSIZE (BUFSIZE - HEADERSIZE)
#define WINDOWSIZE 8

//
// Remote command task
//
RunCommandTask::RunCommandTask(ServerInstance *server, LaunchSettings *launcher,
                               const AttributeMap &subs) :
    TermTask(server, false),
    m_streaming(launcher->outputType() || launcher->inputType()),
    m_buffered(launcher->buffered()),
    m_needInfile(launcher->inputType() == InOutFile),
    m_needOutfile(launcher->outputType() == InOutFile),
    m_subs(subs)
{
    (m_launcher = launcher)->takeReference();

    m_typeStr = TR_TASKTYPE1;
    m_typeIcon = ICON_TASKTYPE_REMOTE_COMMAND;
    m_fromStr = TR_TASKOBJ1;
    m_sourceStr = launcher->name().isEmpty() ?
        TR_TASKOBJ2 + launcher->commandStr() :
        TR_TASKOBJ3 + launcher->name();

    uint32_t command = htole32(TSQ_TASK_INPUT);
    uint32_t status = htole32(Tsq::TaskRunning);

    m_buf = new char[BUFSIZE];
    memcpy(m_buf, &command, 4);
    memcpy(m_buf + 8, serverId().buf, 16);
    memcpy(m_buf + 24, g_listener->id().buf, 16);
    memcpy(m_buf + 40, m_taskId.buf, 16);
    memcpy(m_buf + 56, &status, 4);
    m_ptr = m_buf + HEADERSIZE;

    m_infile = launcher->inputFile();
    m_outfile = launcher->outputFile();
    m_config = launcher->outputConfig();

    if (m_launcher->outputType() == InOutDialog)
        disableStatusPopup();
}

inline void
RunCommandTask::closeInput()
{
    delete m_notifier;
    m_notifier = nullptr;
    close(m_ifd);
    m_ifd = -1;
}

void
RunCommandTask::closefd()
{
    if (m_ifd != -1) {
        delete m_notifier;
        close(m_ifd);
        m_ifd = -1;
    }
    if (m_ofd != -1) {
        close(m_ofd);
        m_ofd = -1;
    }
    m_data.clear();
    m_data.squeeze();
}

RunCommandTask::~RunCommandTask()
{
    closefd();
    m_launcher->putReference();
    delete [] m_buf;
}

void
RunCommandTask::pushStart()
{
    LaunchParams lp = m_launcher->getParams(m_subs);

    Tsq::ProtocolMarshaler m(TSQ_RUN_COMMAND);
    m.addBytes(m_buf + 8, 48);
    m.addNumberPair(m_streaming * PAYLOADSIZE, WINDOWSIZE);
    m.addStringPair(Tsq::attr_COMMAND_COMMAND, lp.cmd.toStdString());
    m.addStringPair(Tsq::attr_COMMAND_ENVIRON, lp.env.join('\x1f').toStdString());
    m.addStringPair(Tsq::attr_COMMAND_STARTDIR, lp.dir.toStdString());

    m_server->conn()->push(m.resultPtr(), m.length());
}

void
RunCommandTask::pushAck()
{
    Tsq::ProtocolMarshaler m(TSQ_TASK_INPUT);
    m.addBytes(m_buf + 8, 48);
    m.addNumber(Tsq::TaskAcking);
    m.addNumber64(m_received);
    m_server->conn()->push(m.resultPtr(), m.length());
}

void
RunCommandTask::pushCancel(int code)
{
    Tsq::ProtocolMarshaler m(TSQ_CANCEL_TASK);
    m.addBytes(m_buf + 8, 48);
    m_server->conn()->push(m.resultPtr(), m.length());

    if (code)
        fail(strerror(code));
}

inline void
RunCommandTask::setDialogFailure()
{
    if (m_dialog)
        m_dialog->setFailure(statusStr());
}

bool
RunCommandTask::openInputFile()
{
    m_ifd = open(pr(m_infile), O_RDONLY|O_CLOEXEC|O_NOCTTY);
    if (m_ifd < 0) {
        fail(strerror(errno));
        return false;
    }
    return true;
}

bool
RunCommandTask::openOutputFile()
{
    std::string tmp = m_outfile.toStdString();

    // Overwrite scenarios
    if (osFileExists(tmp.c_str())) {
        switch (m_config) {
        case Tsq::TaskOverwrite:
            break;
        case Tsq::TaskAsk:
            setQuestion(Tsq::TaskOverwriteRenameQuestion, TR_ASK1);
            return false;
        case Tsq::TaskRename:
            m_ofd = osOpenRenamedFile(tmp, 0644);
            m_outfile = QString::fromStdString(tmp);
            goto out;
        default:
            closefd();
            fail(strerror(EEXIST));
            return false;
        }
    }

    m_ofd = open(tmp.c_str(), O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOCTTY, 0644);
out:
    if (m_ofd < 0) {
        closefd();
        fail(strerror(errno));
        return false;
    }
    return true;
}

void
RunCommandTask::start(TermManager *manager)
{
    disconnect(m_mocStart);

    // We handle FileDialogs ourself, unlike other tasks
    if (m_needInfile && m_infile == g_str_PROMPT_PROFILE) {
        m_mocStart = connect(m_server, SIGNAL(destroyed()), SLOT(deleteLater()));
        auto *box = openBox(TR_TEXT1, manager->parentWidget());
        box->connect(this, SIGNAL(destroyed()), SLOT(deleteLater()));
        connect(box, &QDialog::finished, this, [=](int result) {
            if (result == QDialog::Accepted) {
                m_infile = box->selectedFiles().value(0);
                start(manager);
            } else {
                deleteLater();
            }
        });
        box->show();
        return;
    }
    if (m_needOutfile && m_outfile == g_str_PROMPT_PROFILE) {
        m_mocStart = connect(m_server, SIGNAL(destroyed()), SLOT(deleteLater()));
        auto *box = saveBox(TR_TEXT2, manager->parentWidget());
        box->connect(this, SIGNAL(destroyed()), SLOT(deleteLater()));
        connect(box, &QDialog::finished, this, [=](int result) {
            if (result == QDialog::Accepted) {
                m_outfile = box->selectedFiles().value(0);
                start(manager);
            } else {
                deleteLater();
            }
        });
        box->show();
        return;
    }

    if (!TermTask::doStart(manager) ||
        (m_needInfile && !openInputFile()) ||
        (m_needOutfile && !openOutputFile()))
        return;

    // Bring up status dialog now
    if (m_launcher->outputType() == InOutDialog) {
        m_dialog = new CommandStatusDialog(m_launcher, m_server);
        connect(m_dialog, &QObject::destroyed, this, [this]{ m_dialog = nullptr; });
        m_dialog->bringUp();
    }

    // Write task start (this can fail or finish)
    pushStart();
}

void
RunCommandTask::readInput(int fd)
{
    ssize_t rc;

    if (m_sent - m_acked >= WINDOWSIZE * PAYLOADSIZE) {
        // Stop and wait for ack message
        m_running = false;
        m_notifier->setEnabled(false);
    }
    else if ((rc = read(fd, m_ptr, PAYLOADSIZE)) >= 0) {
        uint32_t length = htole32(HEADERSIZE - 8 + rc);
        memcpy(m_buf + 4, &length, 4);
        m_server->conn()->push(m_buf, HEADERSIZE + rc);
        m_sent += rc;
        queueTaskChange();

        if (rc == 0)
            closeInput();
    }
    else if (errno != EINTR) {
        pushCancel(errno);
        closefd();
        setDialogFailure();
    }
}

void
RunCommandTask::writeOutput(const char *buf, size_t len)
{
    if (m_dialog) {
        m_dialog->appendOutput(QByteArray(buf, len));
    }
    else if (m_buffered) {
        m_data.append(buf, len);
    }
    else {
        size_t sent = 0;
        do {
            ssize_t rc = write(m_ofd, buf, len - sent);
            if (rc < 0) {
                if (errno == EINTR)
                    continue;

                pushCancel(errno);
                closefd();
                return;
            }
            sent += rc;
            buf += rc;
        } while (sent < len);
    }

    m_received += len;
    if (m_chunks < m_received / PAYLOADSIZE) {
        m_chunks = m_received / PAYLOADSIZE;
        pushAck();
    }
    queueTaskChange();
}

void
RunCommandTask::handleStart(int pid)
{
    if (m_launcher->outputType() != InOutNone)
        pushAck();

    if (m_ifd != -1) {
        m_notifier = new QSocketNotifier(m_ifd, QSocketNotifier::Read, this);
        connect(m_notifier, SIGNAL(activated(int)), SLOT(readInput(int)));
        m_running = true;
    }

    m_sinkStr = TR_TASKOBJ4 + QString::number(pid);
    emit taskChanged();
    if (m_dialog)
        m_dialog->setPid(pid);
}

void
RunCommandTask::handleFinish()
{
    TermManager *manager = g_listener->activeManager();

    switch (m_launcher->outputType()) {
    case InOutDialog:
        if (m_dialog)
            m_dialog->setFinished();
        break;
    case InOutWrite:
        if (manager)
            manager->actionWriteText(m_data, g_mtstr);
        break;
    case InOutAction:
        if (m_data.endsWith('\n'))
            m_data.chop(1);
        if (manager && manager->validateSlot(m_data))
            manager->invokeSlot(m_data);
        break;
    case InOutCopy:
        finishCopy(m_data);
        closefd();
        return;
    }

    closefd();
    finish();
    if (m_needOutfile)
        launch(m_outfile);
}

void
RunCommandTask::handleOutput(Tsq::ProtocolUnmarshaler *unm)
{
    if (finished())
        return;

    switch (unm->parseNumber()) {
    case Tsq::TaskAcking:
        if (m_notifier) {
            m_acked = unm->parseNumber64();
            m_running = true;
            m_notifier->setEnabled(!m_throttled);
        }
        break;
    case Tsq::TaskRunning:
        writeOutput(unm->remainingBytes(), unm->remainingLength());
        break;
    case Tsq::TaskStarting:
        handleStart(unm->parseNumber());
        break;
    case Tsq::TaskFinished:
        handleFinish();
        break;
    case Tsq::TaskError:
        closefd();
        unm->parseNumber64();
        fail(QString::fromStdString(unm->parseString()));
        setDialogFailure();
        break;
    default:
        break;
    }
}

void
RunCommandTask::handleAnswer(int answer)
{
    if (!finished()) {
        clearQuestion();

        switch (m_config = answer) {
        case Tsq::TaskOverwrite:
        case Tsq::TaskRename:
            if (openOutputFile())
                // Write task start
                pushStart();
            break;
        default:
            closefd();
            TermTask::cancel();
            break;
        }
    }
}

void
RunCommandTask::cancel()
{
    closefd();

    // Write task cancel
    if (!finished())
        pushCancel(0);

    TermTask::cancel();

    setDialogFailure();
}

void
RunCommandTask::handleDisconnect()
{
    closefd();
}

void
RunCommandTask::handleThrottle(bool throttled)
{
    m_throttled = throttled;
    m_notifier->setEnabled(!throttled && m_running);
}

bool
RunCommandTask::clonable() const
{
    return m_target;
}

TermTask *
RunCommandTask::clone() const
{
    return new RunCommandTask(m_server, m_launcher, m_subs);
}

QString
RunCommandTask::launchfile() const
{
    return m_needOutfile && succeeded() ? m_outfile : g_mtstr;
}

void
RunCommandTask::getDragData(QMimeData *data) const
{
    if (m_needOutfile && succeeded()) {
        data->setText(m_outfile);
        data->setUrls(QList<QUrl>{ QUrl::fromLocalFile(m_outfile) });
    }
}

//
// Local command task
//
LocalCommandTask::LocalCommandTask(LaunchSettings *launcher, const AttributeMap &subs) :
    RunCommandTask(nullptr, launcher, subs)
{
    setNoTarget();

    m_typeStr = TR_TASKTYPE2;
    m_typeIcon = ICON_TASKTYPE_LOCAL_COMMAND;

    // discard header, use a larger buffer size
    delete [] m_buf;
    m_buf = new char[FILE_BUFSIZE];
    m_ptr = m_buf;
}

void
LocalCommandTask::closefd()
{
    if (m_fd != -1) {
        delete m_readNotifier;
        delete m_writeNotifier;
        close(m_fd);
        m_fd = -1;
    }
}

LocalCommandTask::~LocalCommandTask()
{
    closefd();
}

void
LocalCommandTask::pushStart()
{
    int pid;

    try {
        auto lp = m_launcher->getParams(m_subs, '\0');
        ForkParams params;
        params.command = lp.cmd.toStdString();
        params.env = lp.env.join('\0').toStdString();
        params.dir = lp.dir.toStdString();
        osRelativeToHome(params.dir);
        params.daemon = false;
        params.devnull = !m_streaming;
        m_fd = osForkProcess(params, &pid);
        ReaperThread::launchReaper(pid);
    }
    catch (const std::exception &e) {
        RunCommandTask::closefd();
        fail(e.what());
        setDialogFailure();
        return;
    }

    if (m_launcher->outputType() != InOutNone) {
        m_readNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(m_readNotifier, SIGNAL(activated(int)), SLOT(readCommand()));
    }
    if (m_launcher->inputType() != InOutNone) {
        m_writeNotifier = new QSocketNotifier(m_fd, QSocketNotifier::Write, this);
        connect(m_writeNotifier, SIGNAL(activated(int)), SLOT(writeCommand()));
        m_writeNotifier->setEnabled(false);
    }

    handleStart(pid);

    if (!m_readNotifier && !m_writeNotifier)
        finish();
}

void
LocalCommandTask::pushAck()
{}

void
LocalCommandTask::pushCancel(int code)
{
    closefd();

    if (code)
        fail(strerror(code));
}

void
LocalCommandTask::readInput(int fd)
{
    m_notifier->setEnabled(false);

    ssize_t rc = read(fd, m_ptr, FILE_BUFSIZE);
    switch (rc) {
    default:
        m_buffered = rc;
        m_writeNotifier->setEnabled(true);
        break;
    case 0:
        closeInput();
        shutdown(m_fd, SHUT_WR);
        break;
    case -1:
        pushCancel(errno);
        RunCommandTask::closefd();
        setDialogFailure();
    }
}

void
LocalCommandTask::writeCommand()
{
    do {
        ssize_t rc = write(m_fd, m_ptr, m_buffered);
        if (rc < 0) {
            switch (errno) {
            case EAGAIN:
                return;
            case EINTR:
                continue;
            default:
                pushCancel(errno);
                RunCommandTask::closefd();
                setDialogFailure();
                return;
            }
        }
        m_ptr += rc;
        m_buffered -= rc;
        m_sent += rc;
        queueTaskChange();
    } while (m_buffered);

    m_ptr = m_buf;
    m_writeNotifier->setEnabled(false);
    m_notifier->setEnabled(true);
}

void
LocalCommandTask::readCommand()
{
    char buf[READER_BUFSIZE];
    ssize_t rc = read(m_fd, buf, sizeof(buf));
    switch (rc) {
    default:
        writeOutput(buf, rc);
        break;
    case 0:
        closefd();
        handleFinish();
        break;
    case -1:
        if (errno != EINTR && errno != EAGAIN) {
            pushCancel(errno);
            RunCommandTask::closefd();
            setDialogFailure();
        }
    }
}

bool
LocalCommandTask::clonable() const
{
    return true;
}

TermTask *
LocalCommandTask::clone() const
{
    return new LocalCommandTask(m_launcher, m_subs);
}

void
LocalCommandTask::notifySend(const QString &summary, const QString &body)
{
    static bool s_searched;
    if (!s_searched && !(s_searched = osFindBin("notify-send"))) {
        qCWarning(lcSettings) << "notify-send executable not found on the PATH";
        return;
    }

    QStringList cmd = {
        A("notify-send"), A("notify-send"), A("-a"), APP_NAME, summary
    };
    if (!body.isEmpty()) {
        cmd += body;
    }

    try {
        int pid;
        ForkParams params;
        params.command = cmd.join('\0').toStdString();
        params.dir = '/';
        params.daemon = false;
        params.devnull = true;
        osForkProcess(params, &pid);
        ReaperThread::launchReaper(pid);
    }
    catch (const std::exception &e) {
        // do nothing
    }
}
