// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/machine.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE
namespace Tsq { class RawProtocol; }

class ReaderConnection final: public QObject, public Tsq::ProtocolCallback
{
    Q_OBJECT

private:
    Tsq::RawProtocol *m_machine;
    QSocketNotifier *m_notifier = nullptr;
    int m_fd = -1;

    unsigned m_pipeCommand;

    void stop();

private slots:
    void handleConnection();

public:
    ReaderConnection();
    ~ReaderConnection();

    inline int fd() { return m_fd; }

    void start(int fd);
    void pushPipeMessage(unsigned code, const std::string &str);

public:
    bool protocolCallback(uint32_t command, uint32_t length, const char *body);
    void writeFd(const char *buf, size_t len);
    void eofCallback(int errnum);

private:
    void wirePipe(uint32_t command, const std::string &spec);
    void wireList();
    void wireAction(const QByteArray &spec);
};
