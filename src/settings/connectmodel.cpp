// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/simpleitem.h"
#include "connectmodel.h"
#include "connect.h"
#include "settings.h"
#include "global.h"
#include "base/infoanim.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QMouseEvent>

#define CONNECT_COLUMN_ACTIVE   0
#define CONNECT_COLUMN_NAME     1
#define CONNECT_COLUMN_FAVORITE 2
#define CONNECT_COLUMN_TYPE     3
#define CONNECT_COLUMN_SERVER   4
#define CONNECT_COLUMN_COMMAND  5
#define CONNECT_N_COLUMNS       6

ConnectModel::ConnectModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    m_conns = g_settings->conns();
    for (auto conn: qAsConst(m_conns)) {
        conn->activate();
    }

    connect(g_settings, SIGNAL(connectionAdded(int)), SLOT(handleItemAdded(int)));
    connect(g_settings, SIGNAL(connectionUpdated(int)), SLOT(handleItemChanged(int)));
    connect(g_settings, SIGNAL(connectionRemoved(int)), SLOT(handleItemRemoved(int)));
}

QModelIndex
ConnectModel::indexOf(ConnectSettings *conn) const
{
    return index(m_conns.indexOf(conn), 0);
}

void
ConnectModel::handleAnimation(intptr_t row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, CONNECT_N_COLUMNS - 1);
    emit dataChanged(start, end, QVector<int>(1, Qt::BackgroundRole));
}

void
ConnectModel::startAnimation(int row)
{
    auto *conn = m_conns[row];
    auto *animation = conn->animation();
    if (!animation) {
        animation = conn->createAnimation(2);
        connect(animation, SIGNAL(animationSignal(intptr_t)), SLOT(handleAnimation(intptr_t)));
    }
    animation->startColorName(conn->activeColor());
}

void
ConnectModel::handleItemAdded(int row)
{
    beginInsertRows(QModelIndex(), row, row);
    auto *conn = g_settings->conns().at(row);
    m_conns.insert(row, conn);
    conn->activate();
    endInsertRows();
    startAnimation(row);
}

void
ConnectModel::handleItemChanged(int row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, CONNECT_N_COLUMNS - 1);
    emit dataChanged(start, end);
    startAnimation(row);
}

void
ConnectModel::handleItemRemoved(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_conns.removeAt(row);
    endRemoveRows();
}

/*
 * Model functions
 */
int
ConnectModel::columnCount(const QModelIndex &parent) const
{
    return CONNECT_N_COLUMNS;
}

int
ConnectModel::rowCount(const QModelIndex &parent) const
{
    return m_conns.size();
}

QModelIndex
ConnectModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row < m_conns.size())
        return createIndex(row, column, (void *)m_conns.at(row));
    else
        return QModelIndex();
}

QVariant
ConnectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case CONNECT_COLUMN_ACTIVE:
            return tr("Active", "heading");
        case CONNECT_COLUMN_NAME:
            return tr("Name", "heading");
        case CONNECT_COLUMN_FAVORITE:
            return tr("Favorite", "heading");
        case CONNECT_COLUMN_TYPE:
            return tr("Type", "heading");
        case CONNECT_COLUMN_SERVER:
            return tr("Launch From", "heading");
        case CONNECT_COLUMN_COMMAND:
            return tr("Command", "heading");
        }

    return QVariant();
}

QVariant
ConnectModel::data(const QModelIndex &index, int role) const
{
    const auto *conn = (const ConnectSettings *)index.internalPointer();
    if (conn)
        switch (role) {
        case Qt::UserRole:
            if (index.column() == CONNECT_COLUMN_FAVORITE)
                return (conn->anonymous() && !conn->reserved()) ?
                    -1 : conn->isfavorite();
            // fallthru
        case Qt::DisplayRole:
            switch (index.column()) {
            case CONNECT_COLUMN_ACTIVE:
                if (!conn->isbatch())
                    return conn->active() ? tr("Yes") : tr("No");
                break;
            case CONNECT_COLUMN_NAME:
                return conn->name();
            case CONNECT_COLUMN_TYPE:
                return conn->typeStr();
            case CONNECT_COLUMN_SERVER:
                return conn->launchName();
            case CONNECT_COLUMN_COMMAND:
                return conn->commandStr();
            }
            break;
        case Qt::ForegroundRole:
            if (index.column() == CONNECT_COLUMN_ACTIVE)
                return g_global->color(conn->activeColor());
            break;
        case Qt::BackgroundRole:
            if (conn->animation())
                return conn->animation()->colorVariant();
            break;
        case Qt::DecorationRole:
            switch (index.column()) {
            case CONNECT_COLUMN_ACTIVE:
                return conn->activeIcon();
            case CONNECT_COLUMN_NAME:
                return conn->nameIcon();
            case CONNECT_COLUMN_TYPE:
                return conn->typeIcon();
            }
            break;
        }

    return QVariant();
}

Qt::ItemFlags
ConnectModel::flags(const QModelIndex &index) const
{
    if (index.column() == CONNECT_COLUMN_ACTIVE)
        return Qt::ItemIsEnabled;
    else
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

//
// View
//
ConnectView::ConnectView()
{
    m_model = new ConnectModel(this);
    m_filter = new QSortFilterProxyModel(this);
    m_filter->setSortRole(Qt::UserRole);
    m_filter->setDynamicSortFilter(false);
    m_filter->setSourceModel(m_model);

    connect(g_settings, SIGNAL(connectionAdded(int)), m_filter, SLOT(invalidate()));

    QItemSelectionModel *m = selectionModel();
    setModel(m_filter);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(CONNECT_COLUMN_COMMAND, Qt::AscendingOrder);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    setItemDelegateForColumn(CONNECT_COLUMN_FAVORITE,
                             new RadioButtonItemDelegate(this, false));

    connect(m_model, &ConnectModel::dataChanged, this, &ConnectView::rowChanged);
}

ConnectSettings *
ConnectView::selectedConn() const
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return nullptr;

    QModelIndex sindex = m_filter->mapToSource(indexes.at(0));
    return (ConnectSettings *)sindex.internalPointer();
}

void
ConnectView::selectConn(ConnectSettings *conn)
{
    QModelIndex sindex = m_model->indexOf(conn);
    selectRow(m_filter->mapFromSource(sindex).row());
}

void
ConnectView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid() && event->button() == Qt::LeftButton)
        switch (index.column()) {
        case CONNECT_COLUMN_FAVORITE:
            break;
        default:
            selectRow(index.row());
            emit launched();
            event->accept();
        }
}

void
ConnectView::handleClicked(const QModelIndex &pindex)
{
    switch (pindex.column()) {
    case CONNECT_COLUMN_FAVORITE:
        QModelIndex sindex = m_filter->mapToSource(pindex);
        bool checked = sindex.data(Qt::UserRole).toBool();
        auto *conn = (ConnectSettings *)sindex.internalPointer();
        g_settings->setFavoriteConn(conn, !checked);
        break;
    }

    selectRow(pindex.row());
}

void
ConnectView::mousePressEvent(QMouseEvent *event)
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
ConnectView::mouseReleaseEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        event->accept();
    } else {
        QTableView::mouseReleaseEvent(event);
    }
}
