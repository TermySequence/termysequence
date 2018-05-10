// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "task.h"

QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE
class ReaderConnection;

//
// Upload to file
//
class UploadFileTask: public TermTask
{
    Q_OBJECT

protected:
    char *m_buf, *m_ptr;
    size_t m_total, m_acked = 0;
    QSocketNotifier *m_notifier;
    int m_fd = -1;

    bool m_running = false;

    void closefd();
    void pushCancel(int code);

private:
    bool m_throttled = false;
    bool m_overwrite;
    unsigned char m_config;

    QString m_infile, m_outfile;

    void setup(bool overwrite);
    virtual void handleStart(const std::string &name);

private slots:
    void handleFile(int fd);
    void handleThrottle(bool throttled);

protected:
    // Used by UploadPipeTask
    UploadFileTask(ServerInstance *server, int fd);

public:
    UploadFileTask(ServerInstance *server, const QString &infile,
                   const QString &outfile, bool overwrite);
    ~UploadFileTask();

    void start(TermManager *manager);
    void handleOutput(Tsq::ProtocolUnmarshaler *unm);
    void handleQuestion(int question);
    void handleAnswer(int answer);
    void handleDisconnect();
    void cancel();

    bool clonable() const;
    TermTask* clone() const;
};

//
// Upload to pipe
//
class UploadPipeTask final: public UploadFileTask
{
private:
    ReaderConnection *m_reader;

    void handleStart(const std::string &name);
    void handleFile(int fd); // override

public:
    UploadPipeTask(ServerInstance *server, ReaderConnection *reader);

    void start(TermManager *manager);
    void handleQuestion(int question);

    bool clonable() const;
};
