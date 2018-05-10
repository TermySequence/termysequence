// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/simpleitem.h"
#include "porteditmodel.h"

#include <QHeaderView>
#include <QMouseEvent>

#define PORT_COLUMN_LOCAL 0
#define PORT_COLUMN_LSPEC 1
#define PORT_COLUMN_RSPEC 2
#define PORT_COLUMN_AUTO  3
#define PORT_N_COLUMNS    4

PortEditModel::PortEditModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

void
PortEditModel::setPortsList(const QStringList &portsList)
{
    PortFwdList tmp(portsList);

    if (m_ports != tmp) {
        beginResetModel();
        m_ports = tmp;
        m_portsList = tmp.toStringList();
        endResetModel();
    }
}

void
PortEditModel::toggleAuto(const QModelIndex &index)
{
    int row = index.row();
    m_ports[row].isauto = !m_ports.at(row).isauto;
    m_ports[row].update();
    m_portsList = m_ports.toStringList();

    QModelIndex start = createIndex(row, PORT_COLUMN_AUTO);
    QModelIndex end = createIndex(row, PORT_COLUMN_AUTO);
    emit dataChanged(start, end);
    emit portsChanged();
}

int
PortEditModel::autoCount() const
{
    int result = 0;

    for (auto &i: m_ports)
        if (i.isauto)
            ++result;

    return result;
}

int
PortEditModel::addRule(const PortFwdRule &rule)
{
    int row = m_ports.size();
    beginInsertRows(QModelIndex(), row, row);
    m_ports.append(rule);
    m_portsList = m_ports.toStringList();
    endInsertRows();
    emit portsChanged();
    return row;
}

void
PortEditModel::removeRule(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_ports.removeAt(row);
    m_portsList.removeAt(row);
    endRemoveRows();
    emit portsChanged();
}

void
PortEditModel::setRule(int row, const PortFwdRule &rule)
{
    m_ports[row] = rule;
    m_portsList = m_ports.toStringList();

    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, PORT_N_COLUMNS - 1);
    emit dataChanged(start, end);
    emit portsChanged();
}

/*
 * Model functions
 */
int
PortEditModel::columnCount(const QModelIndex &parent) const
{
    return PORT_N_COLUMNS;
}

int
PortEditModel::rowCount(const QModelIndex &parent) const
{
    return m_ports.size();
}

QVariant
PortEditModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case PORT_COLUMN_LOCAL:
            return tr("Listener", "heading");
        case PORT_COLUMN_LSPEC:
            return tr("Local Endpoint", "heading");
        case PORT_COLUMN_RSPEC:
            return tr("Remote Endpoint", "heading");
        case PORT_COLUMN_AUTO:
            return tr("Autorun", "heading");
        }

    return QVariant();
}

QVariant
PortEditModel::data(const QModelIndex &index, int role) const
{
    const auto &rule = m_ports.at(index.row());

    if (role == Qt::UserRole && index.column() == PORT_COLUMN_AUTO)
        return rule.isauto;

    if (role == Qt::DisplayRole)
        switch (index.column()) {
        case PORT_COLUMN_LOCAL:
            return rule.islocal ? tr("Local") : tr("Remote");
        case PORT_COLUMN_LSPEC:
            return rule.localStr();
        case PORT_COLUMN_RSPEC:
            return rule.remoteStr();
        }

    return QVariant();
}

Qt::ItemFlags
PortEditModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

//
// View
//
PortEditView::PortEditView(PortEditModel *model) : m_model(model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(model);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(PORT_COLUMN_LSPEC, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(PORT_COLUMN_RSPEC, QHeaderView::Stretch);

    setItemDelegateForColumn(PORT_COLUMN_AUTO,
                             new RadioButtonItemDelegate(this, false));
}

void
PortEditView::handleClicked(const QModelIndex &index)
{
    switch (index.column()) {
    case PORT_COLUMN_AUTO:
        m_model->toggleAuto(index);
        break;
    }

    selectRow(index.row());
}

void
PortEditView::mousePressEvent(QMouseEvent *event)
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
PortEditView::mouseReleaseEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        event->accept();
    } else {
        QTableView::mouseReleaseEvent(event);
    }
}
