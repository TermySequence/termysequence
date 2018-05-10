// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/simpleitem.h"
#include "layoutmodel.h"

#include <QHeaderView>
#include <QMouseEvent>

#define LAYOUT_COLUMN_NAME 0
#define LAYOUT_COLUMN_ENABLED 1
#define LAYOUT_COLUMN_SEPARATOR 2
#define LAYOUT_N_COLUMNS 3

LayoutModel::LayoutModel(TermLayout &layout, QObject *parent) :
    QAbstractTableModel(parent),
    m_layout(layout)
{
}

void
LayoutModel::reloadData()
{
    QModelIndex start = createIndex(0, 0);
    QModelIndex end = createIndex(LAYOUT_N_WIDGETS - 1, LAYOUT_N_COLUMNS - 1);
    emit dataChanged(start, end);
}

void
LayoutModel::toggleEnabled(const QModelIndex &index)
{
    int row = index.row();
    m_layout.toggleEnabled(row);

    QModelIndex start = createIndex(row, LAYOUT_COLUMN_ENABLED);
    QModelIndex end = createIndex(row, LAYOUT_COLUMN_SEPARATOR);
    emit dataChanged(start, end);
    emit modified();
}

void
LayoutModel::toggleSeparator(const QModelIndex &index)
{
    int row = index.row();
    m_layout.toggleSeparator(row);

    QModelIndex start = createIndex(row, LAYOUT_COLUMN_ENABLED);
    QModelIndex end = createIndex(row, LAYOUT_COLUMN_SEPARATOR);
    emit dataChanged(start, end);
    emit modified();
}

void
LayoutModel::swapPosition(int pos1, int pos2)
{
    m_layout.swapPosition(pos1, pos2);

    QModelIndex start = createIndex(pos1, 0);
    QModelIndex end = createIndex(pos2, LAYOUT_N_COLUMNS - 1);
    emit dataChanged(start, end);
    emit modified();
}

/*
 * Model functions
 */
int
LayoutModel::columnCount(const QModelIndex &parent) const
{
    return LAYOUT_N_COLUMNS;
}

int
LayoutModel::rowCount(const QModelIndex &parent) const
{
    return LAYOUT_N_WIDGETS;
}

QVariant
LayoutModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case LAYOUT_COLUMN_NAME:
            return tr("Ordering", "heading");
        case LAYOUT_COLUMN_ENABLED:
            return tr("Display?", "heading");
        case LAYOUT_COLUMN_SEPARATOR:
            return tr("Separator?", "heading");
        }

    return QVariant();
}

QVariant
LayoutModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    int widget = m_layout.itemAt(row);

    switch (index.column()) {
    case LAYOUT_COLUMN_NAME:
        if (role == Qt::DisplayRole)
            switch (widget) {
            case LAYOUT_WIDGET_TERM:
                return tr("Terminal");
            case LAYOUT_WIDGET_MARKS:
                return tr("Marks");
            case LAYOUT_WIDGET_SCROLL:
                return tr("Scrollbar");
            case LAYOUT_WIDGET_MINIMAP:
                return tr("Minimap");
            case LAYOUT_WIDGET_MODTIME:
                return tr("Timing");
            }
        break;
    case LAYOUT_COLUMN_ENABLED:
        if (role == Qt::UserRole)
            return (widget != LAYOUT_WIDGET_TERM) ? m_layout.enabledAt(row) : -1;
        break;
    case LAYOUT_COLUMN_SEPARATOR:
        if (role == Qt::UserRole)
            return (row < LAYOUT_N_WIDGETS - 1) ? m_layout.separatorAt(row) : -1;
        break;
    }

    return QVariant();
}

Qt::ItemFlags
LayoutModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    case LAYOUT_COLUMN_NAME:
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    default:
        return Qt::ItemIsEnabled;
    }
}

//
// View
//
LayoutView::LayoutView(TermLayout &layout)
{
    m_model = new LayoutModel(layout, this);
    connect(m_model, SIGNAL(modified()), SIGNAL(modified()));

    QItemSelectionModel *m = selectionModel();
    setModel(m_model);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    RadioButtonItemDelegate *radioItem = new RadioButtonItemDelegate(this, false);
    setItemDelegateForColumn(LAYOUT_COLUMN_ENABLED, radioItem);
    setItemDelegateForColumn(LAYOUT_COLUMN_SEPARATOR, radioItem);
}

void
LayoutView::moveUp(int row)
{
    if (row > 0) {
        m_model->swapPosition(row - 1, row);
        selectRow(row - 1);
    }
}

void
LayoutView::moveDown(int row)
{
    if (row < LAYOUT_N_WIDGETS - 1) {
        m_model->swapPosition(row, row + 1);
        selectRow(row + 1);
    }
}

void
LayoutView::handleClicked(const QModelIndex &index)
{
    switch (index.column()) {
    case LAYOUT_COLUMN_ENABLED:
        m_model->toggleEnabled(index);
        break;
    case LAYOUT_COLUMN_SEPARATOR:
        m_model->toggleSeparator(index);
        break;
    }

    selectRow(index.row());
}

void
LayoutView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        handleClicked(index);
        event->accept();
    } else {
        QTableView::mousePressEvent(event);
    }
}

void
LayoutView::mouseReleaseEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        event->accept();
    } else {
        QTableView::mouseReleaseEvent(event);
    }
}
