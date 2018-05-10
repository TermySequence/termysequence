// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "server.h"
#include "lib/wire.h"
#include "lib/machine.h"

#include <QHash>
#include <QSet>
#include <queue>

QT_BEGIN_NAMESPACE
class QSocketNotifier;
class QWidget;
QT_END_NAMESPACE
namespace Tsq { class ClientHandshake; }
class ConnectStatusDialog;

class ServerConnection final: public ServerInstance, public Tsq::ProtocolCallback
{
    Q_OBJECT

private:
    Tsq::ProtocolMachine *m_machine = nullptr;
    Tsq::ClientHandshake *m_handshake = nullptr;

    QHash<Tsq::Uuid,ServerInstance*> m_servers;
    QHash<Tsq::Uuid,TermInstance*> m_terms;

    QSocketNotifier *m_readNotifier = nullptr;
    QSocketNotifier *m_writeNotifier = nullptr;
    int m_fd = -1;
    int m_pid = 0;
    int64_t m_timestamp = 0;

    unsigned char m_idleCount = 0;
    bool m_haveServer = false;
    bool m_independent = false;
    bool m_pty = false;
    bool m_raw = true;
    bool m_announced = false;

    bool m_dialogShown = false;
    bool m_failed = false;
    QString m_failureMessage;

    unsigned m_keepalive = 0;
    int m_timerId = 0;

    std::queue<QByteArray> m_outdata;
    QHash<IdBase*,QSet<TermInstance*>> m_throttles;

    ConnectStatusDialog *m_dialog = nullptr;
    QMetaObject::Connection m_mocActivated;

    void closefd();
    void reportConnectionFailed(const QString &message);
    void handleHandshakeHelper(const char *buf, size_t len);

private slots:
    void handleHandshake(int fd);
    void handleConnection(int fd);
    void handleWrite(int fd);

    void handleDialogDestroyed();
    void handleDialogInput(const QString &input);

protected:
    void timerEvent(QTimerEvent *event);

signals:
    void connectionFailed();

public:
    ServerConnection(ConnectSettings *conninfo);
    ~ServerConnection();

    inline const auto& servers() const { return m_servers; }
    inline const auto& terms() const { return m_terms; }

    inline int64_t timestamp() const { return m_timestamp; }
    inline bool independent() const { return m_independent; }
    inline bool dialogShown() const { return m_dialogShown; }
    inline bool failed() const { return m_failed; }
    inline const QString& failureMessage() const { return m_failureMessage; }
    inline const ConnectSettings* connInfo() const { return m_conninfo; }
    const QString& name() const;

    void start(bool transient);
    void start(QWidget *parent);
    void stop();

    void addServer(ServerInstance *server);
    void removeServer(ServerInstance *server, bool remote);
    void stopServer(ServerInstance *server);

    void addTerm(TermInstance *term);
    void removeTerm(TermInstance *term, bool remote);
    void throttleResume(TermInstance *term, bool notify);

    inline void send(const char *buf, size_t len) { m_machine->connSend(buf, len); }
    inline void push(const char *buf, size_t len) { m_machine->connFlush(buf, len); }

public:
    bool protocolCallback(uint32_t command, uint32_t length, const char *body);
    void writeFd(const char *buf, size_t len);
    void eofCallback(int errnum);

private:
    // wire.cpp
    void pushClientAnnounce();
    void wirePlainCommand(uint32_t command, Tsq::ProtocolUnmarshaler &unm);

    void wireServerAnnounce(Tsq::ProtocolUnmarshaler &unm);
    void wireServerAttribute(ServerInstance *server, Tsq::ProtocolUnmarshaler &unm);
    void wireServerRemove(ServerInstance *server, Tsq::ProtocolUnmarshaler &unm);
    void wireTaskOutput(ServerInstance *server, Tsq::ProtocolUnmarshaler &unm);
    void wireTaskQuestion(ServerInstance *server, Tsq::ProtocolUnmarshaler &unm);
    void wireServerCommand(uint32_t command, Tsq::ProtocolUnmarshaler &unm);
    void wireClientAttribute(Tsq::ProtocolUnmarshaler &unm);

    void wireTermAnnounce(Tsq::ProtocolUnmarshaler &unm, bool isTerm);
    void wireTermAttribute(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermRemove(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermBufferCapacity(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermBufferLength(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermSizeChanged(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermCursorMoved(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermMouseMoved(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermImageContent(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermBellRang(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermRowChanged(TermInstance *term, Tsq::ProtocolUnmarshaler &unm, bool pushed);
    void wireTermRegionChanged(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermDirectoryChanged(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermFileChanged(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermFileRemoved(TermInstance *term, Tsq::ProtocolUnmarshaler &unm);
    void wireTermCommand(uint32_t command, Tsq::ProtocolUnmarshaler &unm);

    void wireThrottlePause(Tsq::ProtocolUnmarshaler &unm);
    void wireClientCommand(uint32_t command, Tsq::ProtocolUnmarshaler &unm);
};
