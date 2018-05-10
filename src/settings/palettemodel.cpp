// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/simpleitem.h"
#include "palettemodel.h"

#include <QHeaderView>

PaletteModel::PaletteModel(const Termcolors &tcpal, QObject *parent) :
    QAbstractTableModel(parent),
    m_tcpal(tcpal)
{
}

void
PaletteModel::reloadData()
{
    QModelIndex start = createIndex(0, 0);
    QModelIndex end = createIndex(PALETTE_N_ROWS - 1, PALETTE_N_COLUMNS - 1);
    emit dataChanged(start, end);
}

void
PaletteModel::reloadOne(int v)
{
    QModelIndex start = createIndex(v / PALETTE_N_COLUMNS, v % PALETTE_N_COLUMNS);
    emit dataChanged(start, start);
}

/*
 * Model functions
 */
int
PaletteModel::columnCount(const QModelIndex &parent) const
{
    return PALETTE_N_COLUMNS;
}

int
PaletteModel::rowCount(const QModelIndex &parent) const
{
    return PALETTE_N_ROWS;
}

QVariant
PaletteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            if (section < PALETTE_N_COLUMNS)
                return tr("Color", "heading") + ' ' + QString::number(section);
        } else {
            if (section < PALETTE_N_ROWS)
                return QString::number(section * PALETTE_N_COLUMNS);
        }
    }

    return QVariant();
}

QVariant
PaletteModel::data(const QModelIndex &index, int role) const
{
    int pos = index.row() * PALETTE_N_COLUMNS + index.column();

    if (pos < PALETTE_APP) {
        switch (role) {
        case Qt::UserRole:
            return m_tcpal[pos];
        case Qt::BackgroundRole:
            return QColor(m_tcpal[pos]);
        }
    }

    return QVariant();
}

Qt::ItemFlags
PaletteModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}

//
// View
//
PaletteView::PaletteView(PaletteModel *model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(model);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);

    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ColorItemDelegate *colorItem = new ColorItemDelegate(this);
    setItemDelegate(colorItem);
}
