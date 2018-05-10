// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "exception.h"

StringException::StringException(const QString &message) :
    m_message(message.toUtf8())
{
}

const char *
StringException::what() const noexcept
{
    return m_message.data();
}

CodedException::CodedException(int code, const QString &message) :
    StringException(message),
    m_code(code)
{
}
