// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/simpleitem.h"
#include "profilemodel.h"
#include "profile.h"
#include "settings.h"
#include "base/infoanim.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QMouseEvent>

#define PROFILE_COLUMN_NAME 0
#define PROFILE_COLUMN_DEFAULT 1
#define PROFILE_COLUMN_FAVORITE 2
#define PROFILE_COLUMN_KEYMAP 3
#define PROFILE_COLUMN_REFCOUNT 4
#define PROFILE_N_COLUMNS 5

ProfileModel::ProfileModel(QWidget *parent) :
    QAbstractTableModel(parent)
{
    m_profiles = g_settings->profiles();
    for (auto profile: qAsConst(m_profiles)) {
        profile->activate();
    }

    connect(g_settings, SIGNAL(profileAdded()), SLOT(handleItemAdded()));
    connect(g_settings, SIGNAL(profileUpdated(int)), SLOT(handleItemChanged(int)));
    connect(g_settings, SIGNAL(profileRemoved(int)), SLOT(handleItemRemoved(int)));
}

QModelIndex
ProfileModel::indexOf(ProfileSettings *profile) const
{
    return index(m_profiles.indexOf(profile), 0);
}

void
ProfileModel::handleAnimation(intptr_t row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, PROFILE_N_COLUMNS - 1);
    emit dataChanged(start, end, QVector<int>(1, Qt::BackgroundRole));
}

void
ProfileModel::startAnimation(int row)
{
    auto *profile = m_profiles[row];
    auto *animation = profile->animation();
    if (!animation) {
        animation = profile->createAnimation();
        connect(animation, SIGNAL(animationSignal(intptr_t)), SLOT(handleAnimation(intptr_t)));
    }
    animation->startColor(static_cast<QWidget*>(QObject::parent()));
}

void
ProfileModel::handleItemAdded()
{
    int row = m_profiles.size();
    beginInsertRows(QModelIndex(), row, row);
    auto *profile = g_settings->profiles().back();
    m_profiles.append(profile);
    profile->activate();
    endInsertRows();
    startAnimation(row);
}

void
ProfileModel::handleItemChanged(int row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, PROFILE_N_COLUMNS - 1);
    emit dataChanged(start, end);
    startAnimation(row);
}

void
ProfileModel::handleItemRemoved(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_profiles.removeAt(row);
    endRemoveRows();
}

/*
 * Model functions
 */
int
ProfileModel::columnCount(const QModelIndex &parent) const
{
    return PROFILE_N_COLUMNS;
}

int
ProfileModel::rowCount(const QModelIndex &parent) const
{
    return m_profiles.size();
}

QModelIndex
ProfileModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row < m_profiles.size())
        return createIndex(row, column, (void *)m_profiles.at(row));
    else
        return QModelIndex();
}

QVariant
ProfileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case PROFILE_COLUMN_NAME:
            return tr("Name", "heading");
        case PROFILE_COLUMN_DEFAULT:
            return tr("Default", "heading");
        case PROFILE_COLUMN_FAVORITE:
            return tr("Favorite", "heading");
        case PROFILE_COLUMN_KEYMAP:
            return tr("Keymap", "heading");
        case PROFILE_COLUMN_REFCOUNT:
            return tr("Open Terminals", "heading");
        }

    return QVariant();
}

QVariant
ProfileModel::data(const QModelIndex &index, int role) const
{
    const auto *profile = (const ProfileSettings *)index.internalPointer();
    if (profile)
        switch (role) {
        case Qt::UserRole:
            switch (index.column()) {
            case PROFILE_COLUMN_DEFAULT:
                return profile->isdefault();
            case PROFILE_COLUMN_FAVORITE:
                return profile->isfavorite();
            }
            // fallthru
        case Qt::DisplayRole:
            switch (index.column()) {
            case PROFILE_COLUMN_NAME:
                return profile->name();
            case PROFILE_COLUMN_KEYMAP:
                return profile->keymapName();
            case PROFILE_COLUMN_REFCOUNT:
                return profile->refcount();
            }
            break;
        case Qt::BackgroundRole:
            if (profile->animation())
                return profile->animation()->colorVariant();
            break;
        case Qt::DecorationRole:
            if (index.column() == PROFILE_COLUMN_NAME)
                return profile->nameIcon();
            break;
        }

    return QVariant();
}

Qt::ItemFlags
ProfileModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

//
// View
//
ProfileView::ProfileView()
{
    m_model = new ProfileModel(this);
    m_filter = new QSortFilterProxyModel(this);
    m_filter->setSortRole(Qt::UserRole);
    m_filter->setDynamicSortFilter(false);
    m_filter->setSourceModel(m_model);

    connect(g_settings, SIGNAL(profileAdded()), m_filter, SLOT(invalidate()));

    QItemSelectionModel *m = selectionModel();
    setModel(m_filter);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(PROFILE_COLUMN_NAME, Qt::AscendingOrder);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    setItemDelegateForColumn(PROFILE_COLUMN_DEFAULT,
                             new RadioButtonItemDelegate(this, true));
    setItemDelegateForColumn(PROFILE_COLUMN_FAVORITE,
                             new RadioButtonItemDelegate(this, false));
}

ProfileSettings *
ProfileView::selectedProfile() const
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return nullptr;

    QModelIndex sindex = m_filter->mapToSource(indexes.at(0));
    return (ProfileSettings *)sindex.internalPointer();
}

void
ProfileView::selectProfile(ProfileSettings *profile)
{
    QModelIndex sindex = m_model->indexOf(profile);
    selectRow(m_filter->mapFromSource(sindex).row());
}

void
ProfileView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid() && event->button() == Qt::LeftButton)
        switch (index.column()) {
        case PROFILE_COLUMN_DEFAULT:
        case PROFILE_COLUMN_FAVORITE:
            break;
        default:
            selectRow(index.row());
            emit launched();
            event->accept();
        }
}

void
ProfileView::handleClicked(const QModelIndex &pindex)
{
    QModelIndex sindex = m_filter->mapToSource(pindex);
    auto *profile = (ProfileSettings *)sindex.internalPointer();

    switch (pindex.column()) {
    case PROFILE_COLUMN_DEFAULT:
        g_settings->setDefaultProfile(profile);
        break;
    case PROFILE_COLUMN_FAVORITE:
        bool checked = sindex.data(Qt::UserRole).toBool();
        g_settings->setFavoriteProfile(profile, !checked);
        break;
    }

    selectRow(pindex.row());
}

void
ProfileView::mousePressEvent(QMouseEvent *event)
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
ProfileView::mouseReleaseEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        event->accept();
    } else {
        QTableView::mouseReleaseEvent(event);
    }
}
