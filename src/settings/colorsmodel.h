// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "termcolors.h"

#include <QAbstractTableModel>
#include <QTableView>

#define COLORS_COLUMN_DESCRIPTION 0
#define COLORS_COLUMN_COLOR       1
#define COLORS_COLUMN_TEXT        2
#define COLORS_N_COLUMNS          3

//
// Model
//
class ColorsModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    const Termcolors &m_tcpal;
    QFont m_font;

    int m_offset;
    int m_rows;

    QVariant colorText(QRgb value) const;

public:
    ColorsModel(const Termcolors &tcpal, QObject *parent);

    inline int offset() const { return m_offset; }

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void reloadData();
    void reloadOne(int idx);

    int row2pos(int row) const;
    int pos2row(int pos) const;
};

//
// View
//
class ColorsView final: public QTableView
{
public:
    ColorsView(ColorsModel *model);
};
