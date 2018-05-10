// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QString>
#include <QList>
#include <QVector>
#include <QRgb>

#define LAYOUT_WIDGET_TERM 0
#define LAYOUT_WIDGET_MARKS 1
#define LAYOUT_WIDGET_SCROLL 2
#define LAYOUT_WIDGET_MINIMAP 3
#define LAYOUT_WIDGET_MODTIME 4
#define LAYOUT_N_WIDGETS 5

class TermLayout
{
public:
    typedef std::pair<unsigned,QRgb> Fill;

private:
    QList<int> m_order;
    QList<bool> m_presence;
    QList<bool> m_separator;
    QVector<Fill> m_fills;

    void setDefaults();

public:
    TermLayout();
    TermLayout(const QString &layoutStr, const QString &fillsStr = QString());
    void parseLayout(const QString &layoutStr);
    void parseFills(const QString &fillsStr);
    QString layoutStr() const;
    QString fillsStr() const;

    inline int itemAt(int pos) const { return m_order.at(pos); }
    inline int itemPosition(int item) const { return m_order.indexOf(item); }
    inline bool enabledAt(int pos) const { return m_presence.at(pos); }
    inline bool separatorAt(int pos) const { return m_separator.at(pos); }
    inline bool itemEnabled(int item) const { return enabledAt(itemPosition(item)); }
    inline bool itemSeparator(int item) const { return separatorAt(itemPosition(item)); }

    void toggleEnabled(int pos);
    void toggleSeparator(int pos);
    void swapPosition(int pos1, int pos2);

    int itemMargin(int item, int margin) const;
    int itemExtraWidth(int item, int margin) const;

    inline const auto& fills() const { return m_fills; }
    int addFill(const Fill &fill);
    void removeFill(int row);
};

inline int
TermLayout::itemMargin(int item, int margin) const
{
    return (item != LAYOUT_WIDGET_SCROLL) ? margin : 0;
}
