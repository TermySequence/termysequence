// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

class Size
{
protected:
    int m_w, m_h;

public:
    inline Size() : m_w(0), m_h(0) {};
    inline Size(int w, int h) : m_w(w), m_h(h) {};

    inline int width() const { return m_w; }
    inline int height() const { return m_h; }

    inline void setWidth(int w) { m_w = w; }
    inline void setHeight(int h) { m_h = h; }
public:
    inline bool operator==(const Size &o) const
    { return m_w == o.m_w && m_h == o.m_h; }
    inline bool operator!=(const Size &o) const
    { return m_w != o.m_w || m_h != o.m_h; }
};

class Point
{
private:
    int m_x, m_y;

public:
    inline Point() : m_x(0), m_y(0) {};
    inline Point(int x, int y) : m_x(x), m_y(y) {};

    inline int x() const { return m_x; }
    inline int y() const { return m_y; }
    inline int &rx() { return m_x; }
    inline int &ry() { return m_y; }

    inline void setX(int x) { m_x = x; }
    inline void setY(int y) { m_y = y; }

    inline bool operator==(const Point &p) const { return m_x == p.m_x && m_y == p.m_y; }
    inline bool operator!=(const Point &p) const { return m_x != p.m_x || m_y != p.m_y; }
};

struct Rect: public Size
{
private:
    int m_x, m_y;

public:
    inline Rect() : m_x(0), m_y(0) {};
    inline Rect(int x, int y, int w, int h) : Size(w, h), m_x(x), m_y(y) {};
    inline Rect(const Point &p, const Size &s) : Size(s), m_x(p.x()), m_y(p.y()) {};

    inline int x() const { return m_x; }
    inline int left() const { return m_x; }
    inline int right() const { return m_x + m_w - 1; }

    inline int y() const { return m_y; }
    inline int top() const { return m_y; }
    inline int bottom() const { return m_y + m_h - 1; }

    inline Point topLeft() const { return Point(m_x, m_y); }
    inline bool contains(const Point &p) const
        { return (p.x() >= m_x && p.x() <= right()) && (p.y() >= m_y && p.y() <= bottom()); }

    inline void setLeft(int x) { m_x = x; }
    inline void setRight(int r) { m_w = r - m_x + 1; }
    inline void setTop(int y) { m_y = y; }
    inline void setBottom(int b) { m_h = b - m_y + 1; }

    inline bool operator==(const Rect &r) const
        { return m_x == r.m_x && m_y == r.m_y && m_w == r.m_w && m_h == r.m_h; }
    inline bool operator!=(const Rect &r) const
        { return m_x != r.m_x || m_y != r.m_y || m_w != r.m_w || m_h != r.m_h; }
};
