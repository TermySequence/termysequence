// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QThread>

QT_BEGIN_NAMESPACE
class QMessageBox;
QT_END_NAMESPACE

struct IconWorker;

// type codes:
// 0: interactive icon load
// 1: noninteractive icon load
// 2: update
// 3: install

class IconThread final: public QThread
{
    Q_OBJECT

private:
    IconWorker *m_target;
    int m_type;
    QString m_msg;

    QMessageBox *m_box = nullptr;

private slots:
    void handleFinished();

protected:
    QAtomicInteger<int> m_progress;
    void run();

    void timerEvent(QTimerEvent *event);

public:
    IconThread(QObject *parent, IconWorker *target, int type);

    inline void setMsg(const QString &msg) { m_msg = msg; }
    void start();

    inline int type() const { return m_type; }

    inline int progress() const { return m_progress.load(); }
    inline void incProgress() { ++m_progress; }
};

struct IconWorker {
    virtual void run(IconThread *thread) = 0;
};
