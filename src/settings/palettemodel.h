// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "termcolors.h"

#include <QAbstractTableModel>
#include <QTableView>

#define PALETTE_N_COLUMNS 8
#define PALETTE_N_ROWS    (PALETTE_APP / PALETTE_N_COLUMNS)

//
// Model
//
class PaletteModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    const Termcolors &m_tcpal;

public:
    PaletteModel(const Termcolors &tcpal, QObject *parent);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void reloadData();
    void reloadOne(int idx);
};

//
// View
//
class PaletteView final: public QTableView
{
public:
    PaletteView(PaletteModel *model);
};
