// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "task.h"
#include "settings/port.h"
#include "lib/types.h"

#include <QVector>
#include <unordered_map>
#include <queue>

QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE
struct addrinfo;

class PortFwdTask: public TermTask
{
    Q_OBJECT

protected:
    struct PortFwdState {
        portfwd_t id;
        int fd;
        QSocketNotifier *reader, *writer;
        std::queue<QByteArray> outdata;
        struct addrinfo *a, *p;
        std::string caddr, cport;
        bool special;
    };

    char *m_buf, *m_ptr;

    size_t m_chunks = 0; // m_received, from server
    size_t m_acked = 0; // m_sent, to server

    std::unordered_map<int,PortFwdState*> m_fdmap;
    std::unordered_map<portfwd_t,PortFwdState*> m_idmap;

    bool m_running = true;
    bool m_throttled = false;
    portfwd_t m_nextId = INVALID_PORTFWD;
    PortFwdRule m_config;

    void pushStart(portfwd_t id);
    void pushBytes(portfwd_t id, size_t len);
    void pushAck();

    void watchReads(bool enabled);

    void closefd(portfwd_t id);
    void closefds();

    virtual void handleStart(Tsq::ProtocolUnmarshaler *unm);
    void handleBytes(portfwd_t id, const char *buf, size_t len);

protected slots:
    void handleThrottle(bool throttled);

    void handleRead(int fd);
    void handleWrite(int fd);
    void handleAccept(int fd);

signals:
    void connectionAdded(portfwd_t id);
    void connectionRemoved(portfwd_t id);

public:
    PortFwdTask(ServerInstance *server, const PortFwdRule &config);
    ~PortFwdTask();

    const auto& config() const { return m_config; }

    struct ConnInfo {
        QString caddr, cport;
        portfwd_t id;
    };
    ConnInfo connectionInfo(portfwd_t id) const;
    QVector<ConnInfo> connectionsInfo() const;
    void killConnection(portfwd_t id);

    using TermTask::start;
    void start(TermManager *manager, uint32_t command);
    void handleOutput(Tsq::ProtocolUnmarshaler *unm);
    void handleDisconnect();
    void cancel();

    bool clonable() const;
};
