// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "task.h"
#include <queue>

QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE
class ReaderConnection;

//
// Base class
//
class DownloadTask: public TermTask
{
private:
    virtual void openFile();
    virtual void finishFile() = 0;
    virtual bool writeData(const char *buf, size_t len) = 0;
    virtual void closefd() = 0;

    virtual void handleStart(Tsq::ProtocolUnmarshaler *unm);
    virtual void handleData(const char *buf, size_t len);

    void setup();

protected:
    char m_buf[48];
    size_t m_chunks = 0, m_total;
    QString m_infile;
    unsigned m_mode;

    void pushAck();
    void pushCancel(int code);

protected:
    DownloadTask(ServerInstance *server, const QString &infile);
    DownloadTask(TermInstance *term, const QString &infile);
    DownloadTask(ServerInstance *server);

public:
    void start(TermManager *manager);

    void handleOutput(Tsq::ProtocolUnmarshaler *unm);
    void handleDisconnect();
    void cancel();

    bool clonable() const;
};

//
// Download to file
//
class DownloadFileTask: public DownloadTask
{
private:
    void setup();

    void openFile();
    void finishFile();
    bool writeData(const char *buf, size_t len);
    void closefd();

protected:
    int m_fd = -1;
    bool m_overwrite;
    unsigned char m_config;
    QString m_outfile;

    // Used by DownloadImageTask
    DownloadFileTask(TermInstance *term, const QString &contentId,
                     const QString &outfile, bool overwrite);

public:
    DownloadFileTask(ServerInstance *server, const QString &infile,
                     const QString &outfile, bool overwrite);
    ~DownloadFileTask();

    void handleAnswer(int answer);

    TermTask* clone() const;
    QString launchfile() const;
    void getDragData(QMimeData *data) const;
};

//
// Download to clipboard
//
class CopyFileTask: public DownloadTask
{
protected:
    QByteArray m_data;
    QString m_format;

    void finishFile();
    bool writeData(const char *buf, size_t len);
    void closefd();

    // Used by FetchImageTask
    CopyFileTask(TermInstance *term, const QString &contentId);

public:
    CopyFileTask(ServerInstance *server, const QString &infile, const QString &format);

    TermTask* clone() const;
};

//
// Download to pipe
//
class DownloadPipeTask final: public DownloadTask
{
    Q_OBJECT

private:
    ReaderConnection *m_reader;
    QSocketNotifier *m_readNotifier;
    QSocketNotifier *m_writeNotifier;
    std::queue<QByteArray> m_outdata;
    int m_fd;

    void finishFile();
    bool writeData(const char *buf, size_t len);
    void closefd();

    void handleStart(Tsq::ProtocolUnmarshaler *unm);
    void handleData(const char *buf, size_t len);

private slots:
    void handleFile();

public:
    DownloadPipeTask(ServerInstance *server, ReaderConnection *reader);

    void start(TermManager *manager);

    bool clonable() const;
};
