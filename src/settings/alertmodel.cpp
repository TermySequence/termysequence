// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/simpleitem.h"
#include "alertmodel.h"
#include "alert.h"
#include "settings.h"
#include "global.h"
#include "base/infoanim.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QMouseEvent>

#define ALERT_COLUMN_ACTIVE   0
#define ALERT_COLUMN_NAME     1
#define ALERT_COLUMN_FAVORITE 2
#define ALERT_COLUMN_COND     3
#define ALERT_COLUMN_ACTION   4
#define ALERT_N_COLUMNS       5

AlertModel::AlertModel(QWidget *parent) :
    QAbstractTableModel(parent)
{
    m_alerts = g_settings->alerts();
    for (auto alert: qAsConst(m_alerts)) {
        alert->activate();
    }

    connect(g_settings, SIGNAL(alertAdded()), SLOT(handleItemAdded()));
    connect(g_settings, SIGNAL(alertUpdated(int)), SLOT(handleItemChanged(int)));
    connect(g_settings, SIGNAL(alertRemoved(int)), SLOT(handleItemRemoved(int)));
}

QModelIndex
AlertModel::indexOf(AlertSettings *alert) const
{
    return index(m_alerts.indexOf(alert), 0);
}

void
AlertModel::handleAnimation(intptr_t row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, ALERT_N_COLUMNS - 1);
    emit dataChanged(start, end, QVector<int>(1, Qt::BackgroundRole));
}

void
AlertModel::startAnimation(int row)
{
    auto *alert = m_alerts[row];
    auto *animation = alert->animation();
    if (!animation) {
        animation = alert->createAnimation(2);
        connect(animation, SIGNAL(animationSignal(intptr_t)), SLOT(handleAnimation(intptr_t)));
    }
    animation->startColor(static_cast<QWidget*>(QObject::parent()));
}

void
AlertModel::handleItemAdded()
{
    int row = m_alerts.size();
    beginInsertRows(QModelIndex(), row, row);
    auto *alert = g_settings->alerts().back();
    m_alerts.append(alert);
    alert->activate();
    endInsertRows();
    startAnimation(row);
}

void
AlertModel::handleItemChanged(int row)
{
    QModelIndex start = createIndex(row, 0);
    QModelIndex end = createIndex(row, ALERT_N_COLUMNS - 1);
    emit dataChanged(start, end);
    startAnimation(row);
}

void
AlertModel::handleItemRemoved(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_alerts.removeAt(row);
    endRemoveRows();
}

/*
 * Model functions
 */
int
AlertModel::columnCount(const QModelIndex &parent) const
{
    return ALERT_N_COLUMNS;
}

int
AlertModel::rowCount(const QModelIndex &parent) const
{
    return m_alerts.size();
}

QModelIndex
AlertModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row >= 0 && row < m_alerts.size())
        return createIndex(row, column, (void *)m_alerts.at(row));
    else
        return QModelIndex();
}

QVariant
AlertModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case ALERT_COLUMN_ACTIVE:
            return tr("Active", "heading");
        case ALERT_COLUMN_NAME:
            return tr("Name", "heading");
        case ALERT_COLUMN_FAVORITE:
            return tr("Favorite", "heading");
        case ALERT_COLUMN_COND:
            return tr("Condition", "heading");
        case ALERT_COLUMN_ACTION:
            return tr("Action", "heading");
        }

    return QVariant();
}

QVariant
AlertModel::getActionString(const AlertSettings *alert) const
{
    QString text;
    int count = 0;

    const char *actList[] = {
        "actServer", "actTerm", "actInd", "actNotify", "actFlash",
        "actPush", "actSwitch", "actSlot", "actLaunch"
    };

    for (unsigned i = 0; i < ARRAY_SIZE(actList); ++i)
        if (alert->property(actList[i]).toBool()) {
            text = alert->actionStr(actList[i]);
            ++count;
        }
    if (count > 1)
        text += L(" (+%1)").arg(tr("%n more actions", "text", count - 1));

    return text;
}

QVariant
AlertModel::data(const QModelIndex &index, int role) const
{
    const auto *alert = (const AlertSettings *)index.internalPointer();
    if (alert)
        switch (role) {
        case Qt::UserRole:
            if (index.column() == ALERT_COLUMN_FAVORITE)
                return alert->isfavorite();
            // fallthru
        case Qt::DisplayRole:
            switch (index.column()) {
            case ALERT_COLUMN_ACTIVE:
                return alert->active() ? tr("Yes") : tr("No");
            case ALERT_COLUMN_NAME:
                return alert->name();
            case ALERT_COLUMN_COND:
                return alert->condStr();
            case ALERT_COLUMN_ACTION:
                return getActionString(alert);
            }
            break;
        case Qt::ForegroundRole:
            if (index.column() == ALERT_COLUMN_ACTIVE)
                return g_global->color(alert->activeColor());
            break;
        case Qt::BackgroundRole:
            if (alert->animation())
                return alert->animation()->colorVariant();
            break;
        }

    return QVariant();
}

Qt::ItemFlags
AlertModel::flags(const QModelIndex &index) const
{
    if (index.column() == ALERT_COLUMN_ACTIVE)
        return Qt::ItemIsEnabled;
    else
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

//
// View
//
AlertView::AlertView()
{
    m_model = new AlertModel(this);
    m_filter = new QSortFilterProxyModel(this);
    m_filter->setSortRole(Qt::UserRole);
    m_filter->setDynamicSortFilter(false);
    m_filter->setSourceModel(m_model);

    connect(g_settings, SIGNAL(alertAdded()), m_filter, SLOT(invalidate()));

    QItemSelectionModel *m = selectionModel();
    setModel(m_filter);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setSortingEnabled(true);
    sortByColumn(ALERT_COLUMN_NAME, Qt::AscendingOrder);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    setItemDelegateForColumn(ALERT_COLUMN_FAVORITE,
                             new RadioButtonItemDelegate(this, false));
}

AlertSettings *
AlertView::selectedAlert() const
{
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty())
        return nullptr;

    QModelIndex sindex = m_filter->mapToSource(indexes.at(0));
    return (AlertSettings *)sindex.internalPointer();
}

void
AlertView::selectAlert(AlertSettings *alert)
{
    QModelIndex sindex = m_model->indexOf(alert);
    selectRow(m_filter->mapFromSource(sindex).row());
}

void
AlertView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid() && event->button() == Qt::LeftButton)
        switch (index.column()) {
        case ALERT_COLUMN_FAVORITE:
            break;
        default:
            selectRow(index.row());
            emit launched();
            event->accept();
        }
}

void
AlertView::handleClicked(const QModelIndex &pindex)
{
    switch (pindex.column()) {
    case ALERT_COLUMN_FAVORITE:
        QModelIndex sindex = m_filter->mapToSource(pindex);
        bool checked = sindex.data(Qt::UserRole).toBool();
        auto *alert = (AlertSettings *)sindex.internalPointer();
        g_settings->setFavoriteAlert(alert, !checked);
        break;
    }

    selectRow(pindex.row());
}

void
AlertView::mousePressEvent(QMouseEvent *event)
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
AlertView::mouseReleaseEvent(QMouseEvent* event)
{
    QModelIndex index = indexAt(event->pos());

    if (index.isValid()) {
        event->accept();
    } else {
        QTableView::mouseReleaseEvent(event);
    }
}
