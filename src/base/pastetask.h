// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "task.h"

QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE

//
// Paste from file
//
class PasteFileTask final: public TermTask
{
    Q_OBJECT

private:
    char *m_buf, *m_ptr;

    QSocketNotifier *m_notifier;
    int m_fd;
    bool m_bracketed;
    size_t m_total;

    QString m_fileName;
    TermTask *m_prereq;

    void reallyStart();
    void pushBytes(size_t len);
    void closefd();

private slots:
    void handleThrottle(bool throttled);
    void handleFile(int fd);
    void handlePrereq();

public:
    PasteFileTask(TermInstance *term, const QString &fileName, TermTask *prereq = nullptr);
    ~PasteFileTask();

    void start(TermManager *manager);
    void handleDisconnect();
    void cancel();

    bool clonable() const;
    TermTask* clone() const;
};

//
// Paste from memory
//
class PasteBytesTask final: public TermTask
{
    Q_OBJECT

private:
    char m_buf[40], *m_ptr;
    size_t m_total;

    QByteArray m_data;

    int m_timerId;

    void pushBytes(size_t len);
    void setTimer(bool enabled);

private slots:
    void handleThrottle(bool throttled);

protected:
    void timerEvent(QTimerEvent *event);

public:
    PasteBytesTask(TermInstance *term, const QByteArray &data);

    void start(TermManager *manager);
    void handleDisconnect();
    void cancel();

    bool clonable() const;
    TermTask* clone() const;
};
