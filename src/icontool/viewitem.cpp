// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "viewitem.h"

IconItemDelegate::IconItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

QSize
IconItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(32, 10);
}


WorkItemDelegate::WorkItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

QSize
WorkItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(10);
    return size;
}
