// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/simpleitem.h"
#include "launchmodel.h"
#include "launcher.h"
#include "settings.h"
#include "base/infoanim.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QMouseEvent>

#define LAUNCH_COLUMN_NAME 0
#define LAUNCH_COLUMN_PATTERN 1
#define LAUNCH_COLUMN_DEFAULT 2
#define LAUNCH_COLUMN_FAVORITE 3
#define LAUNCH_COLUMN_TYPE 4
#define LAUNCH_COLUMN_MOUNT 5
#define LAUNCH_COLUMN_COMMAND 6
#define LAUNCH_N_COLUMNS 7

LauncherModel::LauncherModel(QWidget *parent) :
    QAbstractTableModel(parent)
{
    m_launchers = g_settings->launchers();

    connect(g_settings, SIGNAL(launcherAdded()), SLOT(handleItemAdded()));
    connect(g_settings, SIGNAL(launcherUpdated(int)), SLOT(handleItemChanged(int)));
    connect(g_settings, SIGNAL(launcherReplaced(int)), SLOT(handleItemReplaced(int)));
    connect(g_settings, SIGNAL(launcherRemoved(int)), SLOT(handleItemRemoved(int)));
    connect(g_settings, SIGNAL(launcherReload()), SLOT(handleReload()));
}

QModelIndex
LauncherModel::indexOf(LaunchSettings *launcher) const
{
    return index(m_launchers.indexOf(launcher), 0);
}

void
LauncherModel::handleAnimation(intptr_t row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, LAUNCH_N_COLUMNS - 1);
    emit dataChanged(start, end, QVector<int>(1, Qt::BackgroundRole));
}

void
LauncherModel::startAnimation(int row)
{
    auto *launcher = m_launchers[row];
    auto *animation = launcher->animation();
    if (!animation) {
        animation = launcher->createAnimation();
        connect(animation, SIGNAL(animationSignal(intptr_t)), SLOT(handleAnimation(intptr_t)));
    }
    animation->startColor(static_cast<QWidget*>(QObject::parent()));
}

void
LauncherModel::handleItemAdded()
{
    int row = m_launchers.size();
    beginInsertRows(QModelIndex(), row, row);
    auto *launcher = g_settings->launchers().back();
    m_launchers.append(launcher);
    endInsertRows();
    startAnimation(row);
}

void
LauncherModel::handleItemChanged(int row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, LAUNCH_N_COLUMNS - 1);
    emit dataChanged(start, end);
    startAnimation(row);
}

void
LauncherModel::handleItemReplaced(int row)
{
    m_launchers[row] = g_settings->launchers().at(row);
    handleItemChanged(row);
}

void
LauncherModel::handleItemRemoved(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_launchers.removeAt(row);
    endRemoveRows();
}

void
LauncherModel::handleReload()
{
    beginResetModel();
    m_launchers = g_settings->launchers();
    endResetModel();
}

/*
 * Model functions
 */
int
LauncherModel::columnCount(const QModelIndex &parent) const
{
    return LAUNCH_N_COLUMNS;
}

int
LauncherModel::rowCount(const QModelIndex &parent) const
{
    return m_launchers.size();
}

QModelIndex
LauncherModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row < m_launchers.size())
        return createIndex(row, column, (void *)m_launchers.at(row));
    else
        return QModelIndex();
}

QVariant
LauncherModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case LAUNCH_COLUMN_NAME:
            return tr("Name", "heading");
        case LAUNCH_COLUMN_PATTERN:
            return tr("Extensions", "heading");
        case LAUNCH_COLUMN_DEFAULT:
            return tr("Default", "heading");
        case LAUNCH_COLUMN_FAVORITE:
            return tr("Favorite", "heading");
        case LAUNCH_COLUMN_TYPE:
            return tr("Type", "heading");
        case LAUNCH_COLUMN_MOUNT:
            return tr("Mount", "heading");
        case LAUNCH_COLUMN_COMMAND:
            return tr("Command", "heading");
        }

    return QVariant();
}

QVariant
LauncherModel::getTypeString(const LaunchSettings *launcher) const
{
    QString str;

    switch (launcher->launchType()) {
    case LaunchDefault:
        return tr("Desktop Default", "value");
    case LaunchLocalCommand:
        return tr("Local Command", "value");
    case LaunchRemoteCommand:
        return tr("Remote Command", "value");
    case LaunchLocalTerm:
        str = tr("Local Terminal", "value");
        break;
    case LaunchRemoteTerm:
        str = tr("Remote Terminal", "value");
        break;
    case LaunchWriteCommand:
        return tr("Write Command", "value");
    default:
        return QVariant();
    }

    // Terminal types only get here
    if (!launcher->profile().isEmpty())
        str += L(" (%1:%2)").arg(tr("Profile", "value"), launcher->profile());

    return str;
}

QVariant
LauncherModel::getMountString(const LaunchSettings *launcher) const
{
    if (launcher->local()) {
        switch (launcher->mountType()) {
        case MountReadWrite:
            return tr("Writable", "value");
        case MountReadOnly:
            return tr("Read-only", "value");
        }
    }
    return QVariant();
}

QVariant
LauncherModel::getNameString(const LaunchSettings *launcher) const
{
    QString result = launcher->name();
    if (!launcher->config())
        result += L(" (%1)").arg(tr("share", "value"));
    return result;
}

QVariant
LauncherModel::data(const QModelIndex &index, int role) const
{
    const auto *launcher = (const LaunchSettings *)index.internalPointer();
    if (launcher)
        switch (role) {
        case Qt::UserRole:
            switch (index.column()) {
            case LAUNCH_COLUMN_DEFAULT:
                return launcher->isdefault();
            case LAUNCH_COLUMN_FAVORITE:
                return launcher->isfavorite();
            }
            // fallthru
        case Qt::DisplayRole:
            switch (index.column()) {
            case LAUNCH_COLUMN_NAME:
                return getNameString(launcher);
            case LAUNCH_COLUMN_PATTERN:
                return launcher->patternStr();
            case LAUNCH_COLUMN_TYPE:
                return getTypeString(launcher);
            case LAUNCH_COLUMN_MOUNT:
                return getMountString(launcher);
            case LAUNCH_COLUMN_COMMAND:
                return launcher->commandStr();
            }
            break;
        case Qt::BackgroundRole:
            if (launcher->animation())
                return launcher->animation()->colorVariant();
            break;
        case Qt::DecorationRole:
            switch (index.column()) {
            case LAUNCH_COLUMN_NAME:
                return launcher->nameIcon();
            case LAUNCH_COLUMN_TYPE:
                return launcher->typeIcon();
            }
            break;
        }

    return QVariant();
}

Qt::ItemFlags
LauncherModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

//
// View
//
LauncherView::LauncherView()
{
    m_model = new LauncherModel(this);
    m_filter = new QSortFilterProxyModel(this);
    m_filter->setSortRole(Qt::UserRole);
    m_filter->setDynamicSortFilter(false);
    m_filter->setSourceModel(m_model);

    connect(g_settings, SIGNAL(launcherAdded()), m_filter, SLOT(invalidate()));

    QItemSelectionModel *m = selectionModel();
    setModel(m_filter);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(LAUNCH_COLUMN_NAME, Qt::AscendingOrder);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    setItemDelegateForColumn(LAUNCH_COLUMN_DEFAULT,
                             new RadioButtonItemDelegate(this, true));
    setItemDelegateForColumn(LAUNCH_COLUMN_FAVORITE,
                             new RadioButtonItemDelegate(this, false));
}

LaunchSettings *
LauncherView::selectedLauncher() const
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return nullptr;

    QModelIndex sindex = m_filter->mapToSource(indexes.at(0));
    return (LaunchSettings *)sindex.internalPointer();
}

void
LauncherView::selectLauncher(LaunchSettings *launcher)
{
    QModelIndex sindex = m_model->indexOf(launcher);
    selectRow(m_filter->mapFromSource(sindex).row());
}

void
LauncherView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid() && event->button() == Qt::LeftButton)
        switch (index.column()) {
        case LAUNCH_COLUMN_DEFAULT:
        case LAUNCH_COLUMN_FAVORITE:
            break;
        default:
            selectRow(index.row());
            emit launched();
            event->accept();
        }
}

void
LauncherView::handleClicked(const QModelIndex &pindex)
{
    QModelIndex sindex = m_filter->mapToSource(pindex);
    auto *launcher = (LaunchSettings *)sindex.internalPointer();

    switch (pindex.column()) {
    case LAUNCH_COLUMN_DEFAULT:
        g_settings->setDefaultLauncher(launcher);
        break;
    case LAUNCH_COLUMN_FAVORITE:
        bool checked = sindex.data(Qt::UserRole).toBool();
        g_settings->setFavoriteLauncher(launcher, !checked);
        break;
    }

    selectRow(pindex.row());
}

void
LauncherView::mousePressEvent(QMouseEvent *event)
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
LauncherView::mouseReleaseEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        event->accept();
    } else {
        QTableView::mouseReleaseEvent(event);
    }
}
