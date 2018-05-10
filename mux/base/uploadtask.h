// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "taskbase.h"

//
// Upload to file
//
class FileUpload: public TaskBase
{
private:
    size_t m_received = 0;
    size_t m_chunks = 0;

    std::queue<std::string*> m_outdata;

    bool m_succeeded = false;

protected:
    uint32_t m_chunkSize;
    uint32_t m_mode;
    std::string m_fileName;

    void reportStatus(Tsq::TaskStatus status, const char *buf, unsigned len);
    void reportError(int errtype, int errnum);

private:
    virtual bool openfd();
    void closefd();

    void threadMain();
    bool handleWork(const WorkItem &item);
    bool handleFd();
    bool handleData(std::string *data);
    virtual bool handleAnswer(int answer);
    bool handleIdle();

protected:
    // Used by PipeUpload
    FileUpload(Tsq::ProtocolUnmarshaler *unm, unsigned flags);

public:
    FileUpload(Tsq::ProtocolUnmarshaler *unm);
};

//
// Upload to pipe
//
class PipeUpload final: public FileUpload
{
private:
    bool openfd();

    bool handleAnswer(int answer);
    bool handleIdle();

public:
    PipeUpload(Tsq::ProtocolUnmarshaler *unm);
    ~PipeUpload();
};
