// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "taskbase.h"

#include <memory>

class TermInstance;

class ImageDownload final: public TaskBase
{
private:
    std::shared_ptr<const std::string> m_data;

    char *m_buf, *m_ptr;
    uint32_t m_chunkSize, m_windowSize;

    size_t m_sent;
    size_t m_acked;

    int m_savedTimeout;
    bool m_running;

private:
    bool begin();
    void pushBytes(size_t len);

    void threadMain();
    bool handleWork(const WorkItem &item);
    void handleData(std::string *data);
    bool handleIdle();

public:
    ImageDownload(Tsq::ProtocolUnmarshaler *unm, TermInstance *term);
    ~ImageDownload();
};
