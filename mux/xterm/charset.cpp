// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "charset.h"

TermCharset::TermCharset(int left, int right, Charset a, Charset b, Charset c, Charset d):
    m_charsets{{ a, b, c, d }},
    m_left(left),
    m_right(right),
    m_nextLeft(-1)
{
    loadLeft(m_charsets.at(left));
    loadRight(m_charsets.at(right));
}

void
TermCharset::loadLeft(Charset charset)
{
    for (int i = 0; i < 128; ++i)
    {
        const Codepoint c = (*charset)[i];
        m_set[i] = (c != 0) ? c : i;
    }

    m_leftSet = charset;
}

void
TermCharset::loadRight(Charset charset)
{
    for (int i = 0, j = 128; i < 128; ++i, ++j)
    {
        const Codepoint c = (*charset)[i];
        m_set[j] = (c != 0) ? c : j;
    }

    m_rightSet = charset;
}

void
TermCharset::setCharset(int pos, Charset charset)
{
    if (m_charsets.at(pos) != charset) {
        m_charsets[pos] = charset;

        if (m_left == pos)
            loadLeft(charset);
        if (m_right == pos)
            loadRight(charset);
    }
}

void
TermCharset::setCharsets(const Charsets& charsets, int left, int right, int nextLeft)
{
    Charset charset;

    m_charsets = charsets;

    m_left = left;
    charset = m_charsets.at(left);
    if (m_leftSet != charset)
        loadLeft(charset);

    m_right = right;
    charset = m_charsets.at(right);
    if (m_rightSet != charset)
        loadRight(charset);

    m_nextLeft = nextLeft;
}

void
TermCharset::setLeft(int left)
{
    if (m_left != left) {
        m_left = left;

        Charset charset = m_charsets.at(left);
        if (m_leftSet != charset)
            loadLeft(charset);
    }
}

void
TermCharset::setRight(int right)
{
    if (m_right != right) {
        m_right = right;

        Charset charset = m_charsets.at(right);
        if (m_rightSet != charset)
            loadRight(charset);
    }
}

void
TermCharset::setSingleLeft(int left)
{
    m_nextLeft = m_left;
    setLeft(left);
}
