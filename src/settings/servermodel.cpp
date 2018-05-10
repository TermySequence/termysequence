// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "servermodel.h"
#include "servinfo.h"
#include "settings.h"
#include "global.h"
#include "base/infoanim.h"

#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QMouseEvent>

#define SERVER_COLUMN_ACTIVE    0
#define SERVER_COLUMN_USER      1
#define SERVER_COLUMN_HOST      2
#define SERVER_COLUMN_NAME      3
#define SERVER_COLUMN_CONNNAME  4
#define SERVER_COLUMN_TIME      5
#define SERVER_COLUMN_PROFILE   6
#define SERVER_COLUMN_STARTUP   7
#define SERVER_COLUMN_ID        8
#define SERVER_N_COLUMNS        9

ServerModel::ServerModel(QObject *parent) :
    QAbstractTableModel(parent)
{
    m_servers = g_settings->servers();
    for (auto servinfo: qAsConst(m_servers)) {
        servinfo->activate();
    }

    connect(g_settings, SIGNAL(serverAdded()), SLOT(handleItemAdded()));
    connect(g_settings, SIGNAL(serverUpdated(int)), SLOT(handleItemChanged(int)));
    connect(g_settings, SIGNAL(serverRemoved(int)), SLOT(handleItemRemoved(int)));
}

void
ServerModel::handleAnimation(intptr_t row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, SERVER_N_COLUMNS - 1);
    emit dataChanged(start, end, QVector<int>(1, Qt::BackgroundRole));
}

void
ServerModel::startAnimation(int row)
{
    auto *info = m_servers[row];
    auto *animation = info->animation();
    if (!animation) {
        animation = info->createAnimation(2);
        connect(animation, SIGNAL(animationSignal(intptr_t)), SLOT(handleAnimation(intptr_t)));
    }
    animation->startColorName(info->activeColor());
}

void
ServerModel::handleItemAdded()
{
    int row = m_servers.size();
    beginInsertRows(QModelIndex(), row, row);
    m_servers.append(g_settings->servers().back());
    endInsertRows();
    startAnimation(row);
}

void
ServerModel::handleItemChanged(int row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, SERVER_N_COLUMNS - 1);
    emit dataChanged(start, end);
    startAnimation(row);
}

void
ServerModel::handleItemRemoved(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_servers.removeAt(row);
    endRemoveRows();
}

/*
 * Model functions
 */
int
ServerModel::columnCount(const QModelIndex &parent) const
{
    return SERVER_N_COLUMNS;
}

int
ServerModel::rowCount(const QModelIndex &parent) const
{
    return m_servers.size();
}

QModelIndex
ServerModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < m_servers.size())
        return createIndex(row, column, (void *)m_servers.at(row));
    else
        return QModelIndex();
}

QVariant
ServerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case SERVER_COLUMN_ACTIVE:
            return tr("Active", "heading");
        case SERVER_COLUMN_USER:
            return tr("User", "heading");
        case SERVER_COLUMN_HOST:
            return tr("Host", "heading");
        case SERVER_COLUMN_NAME:
            return tr("Name", "heading");
        case SERVER_COLUMN_CONNNAME:
            return tr("Last Seen Via", "heading");
        case SERVER_COLUMN_TIME:
            return tr("Last Seen Date", "heading");
        case SERVER_COLUMN_PROFILE:
            return tr("Profile", "heading");
        case SERVER_COLUMN_STARTUP:
            return tr("Startup", "heading");
        case SERVER_COLUMN_ID:
            return tr("ID", "heading");
        }

    return QVariant();
}

QVariant
ServerModel::data(const QModelIndex &index, int role) const
{
    const auto *info = (const ServerSettings *)index.internalPointer();
    if (info)
        switch (role) {
        case Qt::UserRole:
            if (index.column() == SERVER_COLUMN_TIME)
                return info->lastTime();
            // fallthru
        case Qt::DisplayRole:
            switch (index.column()) {
            case SERVER_COLUMN_ACTIVE:
                return info->active() ? tr("Yes") : tr("No");
            case SERVER_COLUMN_USER:
                return info->user();
            case SERVER_COLUMN_HOST:
                return info->host();
            case SERVER_COLUMN_NAME:
                return info->name();
            case SERVER_COLUMN_CONNNAME:
                return info->connName();
            case SERVER_COLUMN_TIME:
                return info->lastTime().date().toString();
            case SERVER_COLUMN_PROFILE:
                return info->defaultProfile();
            case SERVER_COLUMN_STARTUP:
                return info->startup().size();
            case SERVER_COLUMN_ID:
                return info->idStr();
            }
            break;
        case Qt::ForegroundRole:
            if (index.column() == SERVER_COLUMN_ACTIVE)
                return g_global->color(info->activeColor());
            break;
        case Qt::BackgroundRole:
            if (info->animation())
                return info->animation()->colorVariant();
            break;
        case Qt::DecorationRole:
            switch (index.column()) {
            case SERVER_COLUMN_ACTIVE:
                return info->activeIcon();
            case SERVER_COLUMN_HOST:
                return info->nameIcon();
            case SERVER_COLUMN_CONNNAME:
                return info->connIcon();
            break;
            }
        }

    return QVariant();
}

Qt::ItemFlags
ServerModel::flags(const QModelIndex &index) const
{
    if (index.column() == SERVER_COLUMN_ACTIVE)
        return Qt::ItemIsEnabled;
    else
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

//
// View
//
ServerView::ServerView()
{
    auto *model = new ServerModel(this);
    m_filter = new QSortFilterProxyModel(this);
    m_filter->setSortRole(Qt::UserRole);
    m_filter->setDynamicSortFilter(false);
    m_filter->setSourceModel(model);

    connect(g_settings, SIGNAL(serverAdded()), m_filter, SLOT(invalidate()));

    QItemSelectionModel *m = selectionModel();
    setModel(m_filter);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

ServerSettings *
ServerView::selectedServer() const
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return nullptr;

    QModelIndex sindex = m_filter->mapToSource(indexes.at(0));
    return (ServerSettings *)sindex.internalPointer();
}

void
ServerView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid() && event->button() == Qt::LeftButton) {
        selectRow(index.row());
        emit launched();
        event->accept();
    }
}

void
ServerView::mousePressEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        selectRow(index.row());
        event->accept();
    } else {
        QTableView::mousePressEvent(event);
    }
}

void
ServerView::mouseReleaseEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        event->accept();
    } else {
        QTableView::mouseReleaseEvent(event);
    }
}
