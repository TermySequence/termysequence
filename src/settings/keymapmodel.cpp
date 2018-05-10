// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "keymapmodel.h"
#include "keymap.h"
#include "settings.h"
#include "profile.h"
#include "base/infoanim.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QMouseEvent>

#define KEYMAP_COLUMN_NAME 0
#define KEYMAP_COLUMN_PARENT 1
#define KEYMAP_COLUMN_DESCRIPTION 2
#define KEYMAP_COLUMN_SIZE 3
#define KEYMAP_COLUMN_PROCOUNT 4
#define KEYMAP_COLUMN_KEYCOUNT 5
#define KEYMAP_N_COLUMNS 6

KeymapModel::KeymapModel(QWidget *parent) :
    QAbstractTableModel(parent)
{
    m_keymaps = g_settings->keymaps();
    for (auto keymap: qAsConst(m_keymaps)) {
        keymap->activate();
    }

    // Auto-activate all profiles in order to get accurate refcounts
    connect(g_settings, &TermSettings::profileAdded, []{
        g_settings->profiles().back()->activate();
    });
    for (auto *profile: g_settings->profiles()) {
        profile->activate();
    }

    connect(g_settings, SIGNAL(keymapAdded()), SLOT(handleItemAdded()));
    connect(g_settings, SIGNAL(keymapUpdated(int)), SLOT(handleItemChanged(int)));
    connect(g_settings, SIGNAL(keymapRemoved(int)), SLOT(handleItemRemoved(int)));
}

QModelIndex
KeymapModel::indexOf(TermKeymap *keymap) const
{
    return index(m_keymaps.indexOf(keymap), 0);
}

void
KeymapModel::handleAnimation(intptr_t row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, KEYMAP_N_COLUMNS - 1);
    emit dataChanged(start, end, QVector<int>(1, Qt::BackgroundRole));
}

void
KeymapModel::startAnimation(int row)
{
    auto *keymap = m_keymaps[row];
    auto *animation = keymap->animation();
    if (!animation) {
        animation = keymap->createAnimation();
        connect(animation, SIGNAL(animationSignal(intptr_t)), SLOT(handleAnimation(intptr_t)));
    }
    animation->startColor(static_cast<QWidget*>(QObject::parent()));
}

void
KeymapModel::handleItemAdded()
{
    int row = m_keymaps.size();
    beginInsertRows(QModelIndex(), row, row);
    auto *keymap = g_settings->keymaps().back();
    m_keymaps.append(keymap);
    keymap->activate();
    endInsertRows();
    startAnimation(row);
}

void
KeymapModel::handleItemChanged(int row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, KEYMAP_N_COLUMNS - 1);
    emit dataChanged(start, end);
    startAnimation(row);
}

void
KeymapModel::handleItemRemoved(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_keymaps.removeAt(row);
    endRemoveRows();
}

/*
 * Model functions
 */
int
KeymapModel::columnCount(const QModelIndex &parent) const
{
    return KEYMAP_N_COLUMNS;
}

int
KeymapModel::rowCount(const QModelIndex &parent) const
{
    return m_keymaps.size();
}

QModelIndex
KeymapModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row < m_keymaps.size())
        return createIndex(row, column, (void *)m_keymaps.at(row));
    else
        return QModelIndex();
}

QVariant
KeymapModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case KEYMAP_COLUMN_NAME:
            return tr("Name", "heading");
        case KEYMAP_COLUMN_PARENT:
            return tr("Inherits", "heading");
        case KEYMAP_COLUMN_DESCRIPTION:
            return tr("Description", "heading");
        case KEYMAP_COLUMN_SIZE:
            return tr("Size", "heading");
        case KEYMAP_COLUMN_PROCOUNT:
            return tr("Profiles Using", "heading");
        case KEYMAP_COLUMN_KEYCOUNT:
            return tr("Keymaps Using", "heading");
        }

    return QVariant();
}

QVariant
KeymapModel::data(const QModelIndex &index, int role) const
{
    const auto *keymap = (const TermKeymap *)index.internalPointer();
    if (keymap)
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case KEYMAP_COLUMN_NAME:
                return keymap->name();
            case KEYMAP_COLUMN_PARENT:
                if ((keymap = keymap->parent()))
                    return keymap->name();
                break;
            case KEYMAP_COLUMN_DESCRIPTION:
                return keymap->description();
            case KEYMAP_COLUMN_SIZE:
                return keymap->size();
            case KEYMAP_COLUMN_PROCOUNT:
                return keymap->profileCount();
            case KEYMAP_COLUMN_KEYCOUNT:
                return keymap->keymapCount() - 1;
            }
            break;
        case Qt::BackgroundRole:
            if (keymap->animation())
                return keymap->animation()->colorVariant();
        }

    return QVariant();
}

Qt::ItemFlags
KeymapModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

//
// View
//
KeymapView::KeymapView()
{
    m_model = new KeymapModel(this);
    m_filter = new QSortFilterProxyModel(this);
    m_filter->setDynamicSortFilter(false);
    m_filter->setSourceModel(m_model);

    connect(g_settings, SIGNAL(keymapAdded()), m_filter, SLOT(invalidate()));

    QItemSelectionModel *m = selectionModel();
    setModel(m_filter);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(KEYMAP_COLUMN_NAME, Qt::AscendingOrder);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

TermKeymap *
KeymapView::selectedKeymap() const
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return nullptr;

    QModelIndex sindex = m_filter->mapToSource(indexes.at(0));
    return (TermKeymap *)sindex.internalPointer();
}

void
KeymapView::selectKeymap(TermKeymap *keymap)
{
    QModelIndex sindex = m_model->indexOf(keymap);
    selectRow(m_filter->mapFromSource(sindex).row());
}

void
KeymapView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid() && event->button() == Qt::LeftButton) {
        selectRow(index.row());
        emit launched();
        event->accept();
    }
}

inline void
KeymapView::handleClicked(const QModelIndex &index)
{
    selectRow(index.row());
}

void
KeymapView::mousePressEvent(QMouseEvent *event)
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
KeymapView::mouseReleaseEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        event->accept();
    } else {
        QTableView::mouseReleaseEvent(event);
    }
}
