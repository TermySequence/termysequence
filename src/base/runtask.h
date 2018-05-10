// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "task.h"

#include <QHash>

QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE
class LaunchSettings;
class CommandStatusDialog;

//
// Remote command task
//
class RunCommandTask: public TermTask
{
    Q_OBJECT

protected:
    char *m_buf, *m_ptr;
    QSocketNotifier *m_notifier = nullptr;

    bool m_running = false;
    bool m_throttled = false;
    unsigned char m_config;
    const bool m_streaming, m_buffered;
    const bool m_needInfile, m_needOutfile;

    CommandStatusDialog *m_dialog = nullptr;
    LaunchSettings *m_launcher;
    const AttributeMap m_subs;

private:
    size_t m_chunks = 0, m_acked = 0;

    int m_ifd = -1, m_ofd = -1;
    QString m_infile, m_outfile;
    QByteArray m_data;

    QMetaObject::Connection m_mocStart;

    bool openInputFile();
    bool openOutputFile();

    virtual void pushStart();
    virtual void pushAck();
    virtual void pushCancel(int code);

private slots:
    virtual void readInput(int fd);
    void handleThrottle(bool throttled);

protected:
    void closefd();
    void closeInput();
    void setDialogFailure();

    void handleStart(int pid);
    void handleFinish();
    void writeOutput(const char *buf, size_t len);

public:
    RunCommandTask(ServerInstance *server, LaunchSettings *launcher,
                   const AttributeMap &subs = AttributeMap());
    ~RunCommandTask();

    void start(TermManager *manager);
    void handleOutput(Tsq::ProtocolUnmarshaler *unm);
    void handleAnswer(int answer);
    void handleDisconnect();
    void cancel();

    bool clonable() const;
    TermTask* clone() const;
    QString launchfile() const;
    void getDragData(QMimeData *data) const;
};

//
// Local command task
//
class LocalCommandTask final: public RunCommandTask
{
    Q_OBJECT

private:
    int m_fd = -1;
    QSocketNotifier *m_readNotifier = nullptr;
    QSocketNotifier *m_writeNotifier = nullptr;
    size_t m_buffered = 0;

    void closefd();

    void pushStart();
    void pushAck();
    void pushCancel(int code);

    void readInput(int fd);

private slots:
    void readCommand();
    void writeCommand();

public:
    LocalCommandTask(LaunchSettings *launcher, const AttributeMap &subs);
    ~LocalCommandTask();

    bool clonable() const;
    TermTask* clone() const;

public:
    static void notifySend(const QString &summary, const QString &body);
};
