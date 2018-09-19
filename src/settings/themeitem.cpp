// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "themeitem.h"
#include "thememodel.h"

#include <QPainter>

#define LINE_WIDTH 4

//
// Sample Item
//
ThemeSampleItemDelegate::ThemeSampleItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void
ThemeSampleItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    QPen pen = painter->pen();
    pen.setBrush(option.palette.dark());
    pen.setWidth(LINE_WIDTH);

    painter->save();
    painter->setPen(pen);
    painter->setClipRect(option.rect);
    painter->drawRect(option.rect);
    painter->restore();
}

QSize
ThemeSampleItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setWidth(size.width() + option.fontMetrics.height());
    return size;
}

//
// Name Item
//
ThemeNameItemDelegate::ThemeNameItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void
ThemeNameItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    bool active = option.state & QStyle::State_Selected;
    auto pc = active ? QPalette::HighlightedText : QPalette::Text;
    QColor color = option.palette.color(QPalette::Active, pc);

    QString name = index.data(THEME_ROLE_NAME).toString();
    QString group = index.data(THEME_ROLE_GROUP).toString();
    int h = option.fontMetrics.height();

    QRect nameRect(option.rect);
    nameRect.translate(h, h / 2);
    nameRect.setWidth(option.rect.width() - h);
    nameRect.setHeight(2 * h);

    QRect descRect(option.rect);
    descRect.translate(h, h * 5 / 2);
    descRect.setWidth(option.rect.width() - h);
    descRect.setHeight(h);

    QFont doubleFont = option.font;
    doubleFont.setPointSize(doubleFont.pointSize() * 2);

    painter->save();
    painter->setPen(color);
    painter->drawText(descRect, Qt::AlignLeft|Qt::AlignVCenter, group);
    painter->setFont(doubleFont);
    painter->drawText(nameRect, Qt::AlignLeft|Qt::AlignVCenter, name);
    painter->restore();
}

QSize
ThemeNameItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(1, option.fontMetrics.height() * 4);
}
