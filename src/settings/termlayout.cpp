// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "termlayout.h"
#include "lib/palette.h"

#include <QSet>
#include <algorithm>

#define SEPARATOR_CHAR C('s')
#define MAIN_DELIM ','
#define FILL_DELIM ':'

inline void
TermLayout::setDefaults()
{
    m_order = { 4, 1, 0, 2, 3 };
    m_presence = { 0, 1, 1, 1, 1 };
    m_separator = { 0, 1, 0, 0, 0 };
}

TermLayout::TermLayout()
{
    setDefaults();
}

TermLayout::TermLayout(const QString &layoutStr, const QString &fillsStr)
{
    setDefaults();
    parseLayout(layoutStr);
    parseFills(fillsStr);
}

void
TermLayout::parseFills(const QString &fillsStr)
{
    m_fills.clear();

    for (const auto &str: fillsStr.split(MAIN_DELIM)) {
        QStringList fields = str.split(FILL_DELIM);
        bool ok;
        Fill fill;
        fill.first = fields.value(0).toUInt(&ok);
        if (!ok)
            break;
        fill.second = fields.value(1).toUInt(&ok);
        if (!ok || fill.second >= PALETTE_APP)
            fill.second = PALETTE_DISABLED;

        for (int i = 0, n = m_fills.size(); i < n; ++i)
            if (m_fills[i].first == fill.first) {
                m_fills[i] = fill;
                goto next;
            }

        m_fills.append(fill);
    next:;
    }

    std::sort(m_fills.begin(), m_fills.end());
}

void
TermLayout::parseLayout(const QString &layoutStr)
{
    QSet<int> seenWidgets;
    int pos = 0;
    bool separatorOk = false;

    for (const auto &str: layoutStr.split(MAIN_DELIM)) {
        if (str == SEPARATOR_CHAR) {
            if (!separatorOk)
                goto bad;

            separatorOk = false;
            m_separator[pos - 1] = true;
            continue;
        }

        bool ok;
        int spec = str.toInt(&ok);
        int widget = qAbs(spec);
        if (!ok || widget >= LAYOUT_N_WIDGETS || seenWidgets.contains(widget))
            goto bad;

        seenWidgets.insert(widget);
        m_order[pos] = widget;
        m_presence[pos] = (spec == widget);
        m_separator[pos] = false;
        separatorOk = (++pos < LAYOUT_N_WIDGETS);
    }

    if (!seenWidgets.contains(LAYOUT_WIDGET_TERM))
        goto bad;

    for (int i = 1; i < LAYOUT_N_WIDGETS; ++i)
        if (!seenWidgets.contains(i)) {
            m_order[pos] = i;
            m_presence[pos] = m_separator[pos] = false;
            ++pos;
        }

    return;
bad:
    setDefaults();
}

QString
TermLayout::layoutStr() const
{
    QStringList list;

    for (int i = 0; i < LAYOUT_N_WIDGETS; ++i) {
        int order = m_order.at(i);
        int val = m_presence.at(i) ? order : -order;
        list.append(QString::number(val));
        if (m_separator.at(i))
            list.append(SEPARATOR_CHAR);
    }
    return list.join(MAIN_DELIM);
}

QString
TermLayout::fillsStr() const
{
    QStringList list;

    for (const auto &elt: m_fills) {
        QString fill = QString::number(elt.first);
        if (PALETTE_IS_ENABLED(elt.second))
            fill += FILL_DELIM + QString::number(elt.second);
        list.append(fill);
    }
    return list.join(MAIN_DELIM);
}

void
TermLayout::toggleEnabled(int pos)
{
    if (m_order.at(pos))
        if (!(m_presence[pos] = !m_presence.at(pos)))
            m_separator[pos] = false;
}

void
TermLayout::toggleSeparator(int pos)
{
    if (pos < LAYOUT_N_WIDGETS - 1) {
        m_separator[pos] = !m_separator[pos];

        if (!m_presence[pos])
            m_presence[pos] = true;
    }
}

void
TermLayout::swapPosition(int pos1, int pos2)
{
    m_order.swap(pos1, pos2);
    m_presence.swap(pos1, pos2);
    m_separator.swap(pos1, pos2);
    m_separator.back() = false;
}

int
TermLayout::itemExtraWidth(int item, int margin) const
{
    int rc = 2 * itemMargin(item, margin);

    if (itemSeparator(item))
        rc += margin;

    return rc;
}

int
TermLayout::addFill(const Fill &fill)
{
    for (int i = 0, n = m_fills.size(); i < n; ++i)
        if (m_fills[i].first == fill.first) {
            m_fills[i] = fill;
            return i;
        }

    m_fills.append(fill);
    std::sort(m_fills.begin(), m_fills.end());
    return m_fills.indexOf(fill);
}

void
TermLayout::removeFill(int row)
{
    m_fills.removeAt(row);
}
