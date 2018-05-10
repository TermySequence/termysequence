// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "palette.h"

#include <QAbstractTableModel>
#include <QTableView>
#include <QVector>

//
// Model
//
class DircolorsModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    const TermPalette &m_palette;
    QFont m_font;

    DircolorsDisplay m_dircolors;
    QVector<Dircolors::Entry> m_list;

    QString m_sample, m_blink;

public:
    DircolorsModel(const TermPalette &palette, const QFont &font, QObject *parent);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void reloadData();
};

//
// View
//
class DircolorsView final: public QTableView
{
    Q_OBJECT

private:
    DircolorsModel *m_model;

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

signals:
    void selectedSubstring(int start, int end);
    void appendedSubstring(const QString &str);

public:
    DircolorsView(const TermPalette &palette, const QFont &font);

    inline void reloadData() { m_model->reloadData(); }
};
