// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "base/codepoint.h"
#include <array>

typedef const std::array<Codepoint,128>* Charset;
typedef std::array<Charset,4> Charsets;

class TermCharset
{
private:
    Charsets m_charsets;
    int m_left, m_right;
    int m_nextLeft;

    Charset m_leftSet, m_rightSet;

    Codepoint m_set[256];

    void loadLeft(Charset charset);
    void loadRight(Charset charset);

public:
    TermCharset(int left, int right, Charset a, Charset b, Charset c, Charset d);

    inline const Charsets& charsets() const { return m_charsets; }
    inline int left() const { return m_left; }
    inline int right() const { return m_right; }
    inline int nextLeft() const { return m_nextLeft; }

    void setCharset(int pos, Charset charset);
    void setCharsets(const Charsets& charsets, int left, int right, int nextLeft);
    void setLeft(int pos);
    void setRight(int pos);
    void setSingleLeft(int pos);

    inline Codepoint map(const Codepoint c)
    {
        Codepoint result = (c < 256) ? m_set[c] : c;

        if (m_nextLeft != -1) {
            setLeft(m_nextLeft);
            m_nextLeft = -1;
        }

        return result;
    }
};
