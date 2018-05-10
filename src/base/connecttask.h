// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "task.h"

#include <QSet>
#include <QHash>
#include <QVector>

class ConnectSettings;
class ConnectStatusDialog;

//
// Connect task
//
class ConnectTask final: public TermTask
{
    Q_OBJECT

private:
    ConnectSettings *m_info;
    char m_buf[48];
    Tsq::Uuid m_connId;

    ConnectStatusDialog *m_dialog = nullptr;
    QMetaObject::Connection m_mocServer, m_mocInput;

    QString m_failMsg;
    bool m_finishing = false;
    bool m_failing = false;

    int m_timerId = 0;

    void pushBytes(size_t len);
    void handleMessage(const std::string &str);

    void killDialog();

    void doFinish(ServerInstance *server);
    void handleFinish();
    void handleError(Tsq::ProtocolUnmarshaler *unm);

private slots:
    void handleDialogDestroyed();
    void handleDialogInput(const QString &input);
    void handleServerAdded(ServerInstance *server);

protected:
    void timerEvent(QTimerEvent *event);

public:
    ConnectTask(ServerInstance *server, ConnectSettings *conninfo);
    ~ConnectTask();

    const QString& name() const;

    void start(TermManager *manager);
    void handleOutput(Tsq::ProtocolUnmarshaler *unm);
    void cancel();

    bool clonable() const;
    TermTask* clone() const;
};

//
// Connect batcher
//
class ConnectBatch final: public TermTask
{
    Q_OBJECT

private:
    ConnectSettings *m_info;

    struct BatchEntry {
        ConnectSettings *info;
        QStringList subs;
        int pos;
    };

    QHash<QString,BatchEntry*> m_map;
    QVector<BatchEntry*> m_stack;
    BatchEntry *m_cur;
    int m_done = -1, m_total = 0;

    BatchEntry* traverse(ConnectSettings *cur, QSet<QString> &seen, TermManager *manager);
    void startNext();
    void moveNext();
    void handleTask(ConnectTask *task);

private slots:
    void handleConnectionReady();
    void handleConnectionFailed();
    void handleTaskChanged();

public:
    ConnectBatch(ConnectSettings *conninfo);
    ~ConnectBatch();

    void start(TermManager *manager);
    void cancel();

    bool clonable() const;
    TermTask* clone() const;
};
