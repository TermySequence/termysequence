// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QStyledItemDelegate>

class IconItemDelegate final: public QStyledItemDelegate
{
public:
    IconItemDelegate(QObject *parent);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class WorkItemDelegate final: public QStyledItemDelegate
{
public:
    WorkItemDelegate(QObject *parent);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};
