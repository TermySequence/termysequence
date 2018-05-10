// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "taskbase.h"

//
// Download from file
//
class FileDownload: public TaskBase
{
protected:
    char *m_buf;

private:
    char *m_ptr;
    uint32_t m_chunkSize, m_windowSize;

    size_t m_sent = 0;
    size_t m_acked = 0;

    bool m_running = false;

private:
    void setup(Tsq::ProtocolUnmarshaler *unm);

    virtual bool openfd();

    void threadMain();
    bool handleFd();
    bool handleWork(const WorkItem &item);
    void handleData(std::string *data);
    bool handleIdle();

protected:
    // Used by PipeDownload
    FileDownload(Tsq::ProtocolUnmarshaler *unm, unsigned flags);

public:
    FileDownload(Tsq::ProtocolUnmarshaler *unm);
    ~FileDownload();
};

//
// Download from pipe
//
class PipeDownload final: public FileDownload
{
private:
    unsigned m_mode;

    bool openfd();
    bool handleIdle();

public:
    PipeDownload(Tsq::ProtocolUnmarshaler *unm);
    ~PipeDownload();
};
