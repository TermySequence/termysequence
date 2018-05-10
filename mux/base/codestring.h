// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "codepoint.h"
#include "lib/utf8.h"

#include <vector>

class CodepointList : public std::vector<Codepoint>
{
public:
    inline CodepointList() {}
    inline CodepointList(Codepoint c) : std::vector<Codepoint>(1, c) {}

    bool contains(Codepoint c) const;
};

class Codestring
{
private:
    std::string m_str;

public:
    typedef utf8::unchecked::iterator<std::string::iterator> iterator;
    typedef utf8::unchecked::iterator<std::string::const_iterator> const_iterator;

    inline iterator begin() { return iterator(m_str.begin()); }
    inline const_iterator begin() const { return const_iterator(m_str.begin()); }
    inline iterator end() { return iterator(m_str.end()); }
    inline const_iterator end() const { return const_iterator(m_str.end()); }

public:
    inline Codestring() {};
    inline Codestring(Codepoint c) : m_str(1, (char)c) {};
    inline Codestring(const std::string &str) : m_str(str) {};
    inline Codestring(const char *buf) : m_str(buf) {};

    size_t size() const;
    inline bool empty() const { return m_str.empty(); }
    inline Codepoint front() const { return *begin(); }
    inline Codepoint back() const { return *std::prev(end()); }
    Codepoint at(unsigned pos) const;

    inline void clear() { m_str.clear(); }
    void push_back(Codepoint c);
    void pop_back();
    void pop_front(unsigned count = 1);

    std::string& rstr() { return m_str; }
    inline const std::string& str() const { return m_str; }
    inline const char* c_str() const { return m_str.c_str(); }
    int toUInt() const;

public:
    inline bool operator==(const Codestring &o) const
    { return m_str == o.m_str; }
    inline bool operator!=(const Codestring &o) const
    { return m_str != o.m_str; }
};

inline bool
CodepointList::contains(Codepoint c) const
{
    for (Codepoint i: *this)
        if (i == c)
            return true;

    return false;
}
