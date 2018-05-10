// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "exception.h"
#include "protocol.h"

#include <cstdio>
#include <cstdarg>

namespace Tsq
{
    TsqException::TsqException()
    {
        m_status = TSQ_STATUS_SERVER_ERROR;
    }

    TsqException::TsqException(const char *format, ...)
    {
        m_status = TSQ_STATUS_SERVER_ERROR;
        va_list args;
        va_start(args, format);
        vsnprintf(m_buf, sizeof(m_buf), format, args);
        va_end(args);
    }

    const char *
    TsqException::what() const noexcept
    {
        return m_buf;
    }


    ErrnoException::ErrnoException(int code)
    {
        snprintf(m_buf, sizeof(m_buf), "%s", strerror(code));
    }

    ErrnoException::ErrnoException(const char *message, int code)
    {
        snprintf(m_buf, sizeof(m_buf), "%s: %s", message, strerror(code));
    }

    ErrnoException::ErrnoException(const char *message, const char *arg1, int code)
    {
        snprintf(m_buf, sizeof(m_buf), "%s %s: %s", message, arg1, strerror(code));
    }


    ProtocolException::ProtocolException(int code)
    {
#ifndef NDEBUG
        abort();
#endif
        m_status = TSQ_STATUS_PROTOCOL_ERROR;
        snprintf(m_buf, sizeof(m_buf), "%s", strerror(code));
    }
}
