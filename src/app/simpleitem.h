// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QStyledItemDelegate>

//
// Combo Item
//
class ComboBoxItemDelegate final: public QStyledItemDelegate
{
private:
    bool m_editable;

public:
    ComboBoxItemDelegate(bool editable, QObject *parent);

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
};

//
// Color Item
//
class ColorItemDelegate final: public QStyledItemDelegate
{
private:
    QFont m_font;

public:
    ColorItemDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

//
// Radio Item
//
class RadioButtonItemDelegate final: public QStyledItemDelegate
{
private:
    bool m_isRadio;

public:
    RadioButtonItemDelegate(QObject *parent, bool isRadio);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

//
// Progress Bar
//
class ProgressBarItemDelegate final: public QStyledItemDelegate
{
public:
    ProgressBarItemDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

//
// Svg Item
//
class SvgImageItemDelegate final: public QStyledItemDelegate
{
public:
    SvgImageItemDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};
