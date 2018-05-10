// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/simpleitem.h"
#include "batchmodel.h"
#include "settings.h"
#include "connect.h"
#include "servinfo.h"

#include <QHeaderView>

#define BATCH_COLUMN_CONN       0
#define BATCH_COLUMN_SERVER     1
#define BATCH_N_COLUMNS         2

#define TR_NAME1 TL("settings-name", "Connect directly from client")

//
// Model
//
BatchModel::BatchModel(const QStringList &list, QObject *parent) :
    QAbstractTableModel(parent)
{
    for (auto info: g_settings->conns()) {
        if (!info->anonymous()) {
            info->activate();
            BatchRule *ent = new BatchRule();
            ent->connName = info->name();
            ent->connIcon = info->typeIcon();
            ent->index = m_connList.size();
            m_connList.append(ent);
            m_connMap.insert(ent->connName, ent);
            m_connNames.append(ent->connName);
            m_connIcons.append(ent->connIcon);

            auto *servinfo = g_settings->server(info->launchId());
            if (servinfo) {
                servinfo->activate();
                ent->serverId = servinfo->idStr();
                ent->serverName = servinfo->fullname();
                ent->serverIcon = servinfo->nameIcon();
            }
            else if (!info->isbatch()) {
                ent->serverName = TR_NAME1;
            }
        }
    }

    connect(g_settings, SIGNAL(connectionUpdated(int)), SLOT(handleConnUpdated(int)));
    connect(g_settings, SIGNAL(serverUpdated(int)), SLOT(handleServerUpdated(int)));

    loadRules(list);
}

BatchModel::~BatchModel()
{
    forDeleteAll(m_connList);
}

QStringList
BatchModel::result() const
{
    QStringList result;
    for (const auto *rule: m_ruleset) {
        result.append(rule->connName);
    }
    return result;
}

void
BatchModel::handleConnUpdated(int row)
{
    const auto *info = g_settings->conns().at(row);
    auto i = m_connMap.find(info->name());
    if (i != m_connMap.end()) {
        m_connIcons[(*i)->index] = (*i)->connIcon = info->typeIcon();

        QModelIndex si = index(0, BATCH_COLUMN_CONN);
        QModelIndex ei = index(m_ruleset.size() - 1, BATCH_COLUMN_CONN);
        emit dataChanged(si, ei);
    }
}

void
BatchModel::handleServerUpdated(int row)
{
    const auto *servinfo = g_settings->servers().at(row);
    for (auto *rule: qAsConst(m_connList))
        if (rule->serverId == servinfo->idStr()) {
            rule->serverName = servinfo->fullname();
            rule->serverIcon = servinfo->nameIcon();
        }

    row = 0;
    for (const auto *rule: qAsConst(m_ruleset)) {
        if (rule->serverId == servinfo->idStr()) {
            QModelIndex si = index(row, BATCH_COLUMN_SERVER);
            QModelIndex ei = index(m_ruleset.size() - 1, BATCH_COLUMN_SERVER);
            emit dataChanged(si, ei);
            break;
        }
        ++row;
    }
}

void
BatchModel::moveFirst(int start, int end)
{
    for (int i = 0; i < end - start + 1; ++i)
        m_ruleset.swap(i, start + i);

    QModelIndex si = index(0, 0);
    QModelIndex ei = index(end, BATCH_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
BatchModel::moveUp(int start, int end)
{
    m_ruleset.move(start - 1, end);

    QModelIndex si = index(start - 1, 0);
    QModelIndex ei = index(end, BATCH_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
BatchModel::moveDown(int start, int end)
{
    m_ruleset.move(end + 1, start);

    QModelIndex si = index(start, 0);
    QModelIndex ei = index(end + 1, BATCH_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
BatchModel::moveLast(int start, int end)
{
    int n = end - start + 1;
    int m = m_ruleset.size() - n;
    for (int i = 0; i < n; ++i)
        m_ruleset.swap(m + i, start + i);

    QModelIndex si = index(start, 0);
    QModelIndex ei = index(m_ruleset.size() - 1, BATCH_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
BatchModel::insertRule(int row)
{
    if (!m_connList.isEmpty()) {
        beginInsertRows(QModelIndex(), row, row);
        m_ruleset.insert(row, m_connList.front());
        endInsertRows();
    }
}

void
BatchModel::cloneRule(int row)
{
    auto ent = m_ruleset[row++];
    beginInsertRows(QModelIndex(), row, row);
    m_ruleset.insert(row, ent);
    endInsertRows();
}

void
BatchModel::removeRule(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_ruleset.removeAt(row);
    endRemoveRows();
}

void
BatchModel::loadRules(const QStringList &list)
{
    beginResetModel();
    m_ruleset.clear();
    for (const QString &connName: list) {
        auto i = m_connMap.constFind(connName);
        if (i != m_connMap.end()) {
            m_ruleset.append(*i);
        }
    }
    endResetModel();
}

/*
 * Model functions
 */
int
BatchModel::columnCount(const QModelIndex &parent) const
{
    return BATCH_N_COLUMNS;
}

int
BatchModel::rowCount(const QModelIndex &parent) const
{
    return m_ruleset.size();
}

QModelIndex
BatchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < m_ruleset.size())
        return createIndex(row, column, m_ruleset.at(row));
    else
        return QModelIndex();
}

QVariant
BatchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case BATCH_COLUMN_CONN:
            return tr("Launch This Connection", "heading");
        case BATCH_COLUMN_SERVER:
            return tr("On This Server", "heading");
        }

    return QVariant();
}

QVariant
BatchModel::data(const QModelIndex &index, int role) const
{
    const auto *rule = (const BatchRule *)index.internalPointer();
    if (rule)
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case BATCH_COLUMN_CONN:
                return rule->connName;
            case BATCH_COLUMN_SERVER:
                return rule->serverName;
            }
            break;
        case Qt::DecorationRole:
            switch (index.column()) {
            case BATCH_COLUMN_CONN:
                return rule->connIcon;
            case BATCH_COLUMN_SERVER:
                return rule->serverIcon;
            }
            break;
        case Qt::EditRole:
            switch (index.column()) {
            case BATCH_COLUMN_CONN:
                return rule->index;
            }
            break;
        case Qt::UserRole:
            switch (index.column()) {
            case BATCH_COLUMN_CONN:
                return m_connNames;
            }
            break;
        case Qt::UserRole + 1:
            switch (index.column()) {
            case BATCH_COLUMN_CONN:
                return m_connIcons;
            }
            break;
        }

    return QVariant();
}

Qt::ItemFlags
BatchModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    case BATCH_COLUMN_CONN:
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable;
    default:
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    }
}

bool
BatchModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || index.column() != BATCH_COLUMN_CONN)
        return false;

    m_ruleset[index.row()] = m_connList.at(value.toInt());

    QModelIndex si = this->index(index.row(), 0);
    QModelIndex ei = this->index(index.row(), BATCH_N_COLUMNS - 1);
    emit dataChanged(si, ei);
    return true;
}

//
// View
//
BatchView::BatchView(QAbstractTableModel *model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(model);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    auto choiceItem = new ComboBoxItemDelegate(false, this);
    setItemDelegateForColumn(BATCH_COLUMN_CONN, choiceItem);
}
