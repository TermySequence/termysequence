// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "codestring.h"
#include "attributemap.h"

const std::string g_mtstr;

size_t
Codestring::size() const
{
    std::string::const_iterator i = m_str.begin();
    std::string::const_iterator j = m_str.end();

    return utf8::unchecked::distance(i, j);
}

Codepoint
Codestring::at(unsigned pos) const
{
    std::string::const_iterator i = m_str.begin();
    utf8::unchecked::advance(i, pos);
    return utf8::unchecked::peek_next(&*i);
}

int
Codestring::toUInt() const
{
    int result = 0;
    const char *ptr = m_str.c_str();

    while (result < 10000 && *ptr >= '0' && *ptr <= '9') {
        result = (result * 10) + (*ptr++ - '0');
    }
    return result;
}

void
Codestring::push_back(Codepoint c)
{
    utf8::unchecked::append(c, std::back_inserter(m_str));
}

void
Codestring::pop_back()
{
    std::string::iterator i = std::prev(end()).base();
    std::string::iterator j = m_str.end();

    m_str.erase(i, j);
}

void
Codestring::pop_front(unsigned count)
{
    std::string::iterator i = m_str.begin(), j;
    utf8::unchecked::advance(j = i, count);

    m_str.erase(i, j);
}
