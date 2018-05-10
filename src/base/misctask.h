// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "task.h"

//
// Delete file task
//
class DeleteFileTask final: public TermTask
{
private:
    char m_buf[48];
    unsigned char m_config;
    QString m_fileName;

public:
    DeleteFileTask(ServerInstance *server, const QString &fileName);

    void start(TermManager *manager);
    void handleOutput(Tsq::ProtocolUnmarshaler *unm);
    void handleQuestion(int question);
    void handleAnswer(int answer);
    void cancel();

    bool clonable() const;
    TermTask* clone() const;
};

//
// Rename file task
//
class RenameFileTask final: public TermTask
{
private:
    char m_buf[48];
    unsigned char m_config;
    QString m_infile, m_outfile;

public:
    RenameFileTask(ServerInstance *server, const QString &infile, const QString &outfile);

    void start(TermManager *manager);
    void handleOutput(Tsq::ProtocolUnmarshaler *unm);
    void handleQuestion(int question);
    void handleAnswer(int answer);
    void cancel();

    bool clonable() const;
    TermTask* clone() const;
};
