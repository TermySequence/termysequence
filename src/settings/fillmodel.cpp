// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "fillmodel.h"
#include "filleditor.h"

#include <QHeaderView>

#define FILL_COLUMN_COLUMN 0
#define FILL_COLUMN_COLOR 1
#define FILL_COLUMN_PREVIEW 2
#define FILL_N_COLUMNS 3

#define TR_DIM0 TL("settings-dimension", "Foreground", "value")

FillModel::FillModel(TermLayout &layout, const Termcolors &tcpal, QObject *parent) :
    QAbstractTableModel(parent),
    m_layout(layout),
    m_tcpal(tcpal)
{
}

void
FillModel::reloadData()
{
    beginResetModel();
    endResetModel();
}

/*
 * Model functions
 */
int
FillModel::columnCount(const QModelIndex &parent) const
{
    return FILL_N_COLUMNS;
}

int
FillModel::rowCount(const QModelIndex &parent) const
{
    return m_layout.fills().size();
}

QVariant
FillModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case FILL_COLUMN_COLUMN:
            return tr("Column", "heading");
        case FILL_COLUMN_COLOR:
            return tr("Color", "heading");
        case FILL_COLUMN_PREVIEW:
            return tr("Preview", "heading");
        }

    return QVariant();
}

QVariant
FillModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    auto elt = m_layout.fills().value(row);

    switch (index.column()) {
    case FILL_COLUMN_COLUMN:
        if (role == Qt::DisplayRole)
            return elt.first;
        break;
    case FILL_COLUMN_COLOR:
        if (role == Qt::DisplayRole)
            return PALETTE_IS_ENABLED(elt.second) ?
                QString::number(elt.second) : TR_DIM0;
        break;
    case FILL_COLUMN_PREVIEW:
        if (role == Qt::BackgroundRole)
            return QColor(m_tcpal.at(PALETTE_IS_ENABLED(elt.second) ?
                                     elt.second : PALETTE_APP_FG));
        break;
    }

    return QVariant();
}

Qt::ItemFlags
FillModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    case FILL_COLUMN_PREVIEW:
        return Qt::ItemIsEnabled;
    default:
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
}

//
// View
//
FillView::FillView(TermLayout &layout, const Termcolors &tcpal, const QFont &font) :
    m_layout(layout),
    m_tcpal(tcpal),
    m_font(font)
{
    m_model = new FillModel(layout, m_tcpal, this);
    connect(m_model, SIGNAL(modelReset()), SIGNAL(modelReset()));

    QItemSelectionModel *m = selectionModel();
    setModel(m_model);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void
FillView::addRule()
{
    auto *dialog = new FillEditor(this, m_tcpal, m_font, true);
    connect(dialog, &QDialog::accepted, this, [=]{
        int row = m_layout.addFill(dialog->fill());
        m_model->reloadData();
        selectRow(row);
        emit modified();
    });
    dialog->show();
}

void
FillView::editRule()
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return;

    int row = indexes.at(0).row();
    auto *dialog = new FillEditor(this, m_tcpal, m_font, false);
    dialog->setFill(m_layout.fills().at(row));
    connect(dialog, &QDialog::accepted, this, [=]{
        m_layout.removeFill(row);
        int row = m_layout.addFill(dialog->fill());
        m_model->reloadData();
        selectRow(row);
        emit modified();
    });
    dialog->show();
}

void
FillView::deleteRule()
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return;

    m_layout.removeFill(indexes.at(0).row());
    m_model->reloadData();
    emit modified();
}
