// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <stdexcept>
#include <cerrno>

// Exception codes
#define TSQ_ERR_OBJECTNOTFOUND 0
#define TSQ_ERR_OBJECTEXISTS   1
#define TSQ_ERR_BADOBJECTNAME  2
#define TSQ_ERR_OBJECTREADONLY 3
#define TSQ_ERR_OBJECTINHERIT  4
#define TSQ_ERR_CANNOTSAVE     5

namespace Tsq
{
    class TsqException: public std::exception
    {
    protected:
        char m_buf[128];
        int m_status;

        TsqException();

    public:
        TsqException(const char *format, ...);

        const char *what() const noexcept;
        inline int status() const noexcept { return m_status; }
    };


    class ErrnoException: public TsqException
    {
    public:
        ErrnoException(int code);
        ErrnoException(const char *message, int code);
        ErrnoException(const char *message, const char *arg1, int code);
    };


    class ProtocolException: public TsqException
    {
    public:
        ProtocolException(int code = EBADMSG);
    };
}
