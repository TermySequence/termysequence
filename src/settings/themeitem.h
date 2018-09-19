// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QStyledItemDelegate>
#include <QFont>

//
// Sample Item
//
class ThemeSampleItemDelegate final: public QStyledItemDelegate
{
public:
    ThemeSampleItemDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

//
// Name Item
//
class ThemeNameItemDelegate final: public QStyledItemDelegate
{
public:
    ThemeNameItemDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};
