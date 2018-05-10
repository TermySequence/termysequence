// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "lib/exception.h"

#include <QString>

typedef Tsq::TsqException TsqException;
typedef Tsq::ErrnoException ErrnoException;

class StringException: public std::exception
{
protected:
    QByteArray m_message;

public:
    StringException(const QString &message);

    inline const QByteArray& message() const noexcept { return m_message; }

    const char *what() const noexcept;
};

class CodedException final: public StringException
{
protected:
    int m_code;

public:
    CodedException(int code, const QString &message);

    inline int code() const noexcept { return m_code; }
};
