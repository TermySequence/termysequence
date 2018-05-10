// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "taskbase.h"

#include <queue>
#include <unordered_map>
#include <dirent.h>

class FileMount final: public TaskBase
{
private:
    char *m_buf, *m_ptr;
    size_t m_bufsize;

    struct Req {
        unsigned id;
        Tsq::MountTaskOpcode op;
        std::string name, data;
    };

    std::queue<Req> m_reqs;
    std::unordered_map<std::string,std::pair<int,unsigned>> m_files;
    std::unordered_map<std::string,std::pair<DIR*,unsigned>> m_dirs;
    DIR *m_dir;

    bool m_ro;
    bool m_isdir;

private:
    bool openfd();
    int openSubfd(const char *name, Tsq::MountTaskResult &rc);
    void setBufferSize(size_t size);
    void addNumber(uint32_t value);
    void addNumber64(uint64_t value);
    void addPaddedString(const char *buf, uint32_t len);

    void pushReply(unsigned reqid, unsigned reqcode);

    void threadMain();
    bool handleWork(const WorkItem &item);
    void handleRequest(std::string *data);

    bool handleIdle();
    void handleStat(Req &req);
    void handleOpen(Req &req);
    void handleRead(Req &req, Tsq::ProtocolUnmarshaler *unm);
    void handleWrite(Req &req, Tsq::ProtocolUnmarshaler *unm, bool append);
    void handleClose(Req &req);
    void handleChmod(Req &req, Tsq::ProtocolUnmarshaler *unm);
    void handleTrunc(Req &req, Tsq::ProtocolUnmarshaler *unm);
    void handleTouch(Req &req);
    void handleCreate(Req &req, Tsq::ProtocolUnmarshaler *unm);
    void handleOpendir(Req &req);
    void handleReaddir(Req &req, Tsq::ProtocolUnmarshaler *unm);
    void handleClosedir(Req &req);

public:
    FileMount(Tsq::ProtocolUnmarshaler *unm, bool readonly);
    ~FileMount();
};
