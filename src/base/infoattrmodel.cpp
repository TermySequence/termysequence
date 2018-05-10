// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "infoattrmodel.h"
#include "infoanim.h"

#include <QHeaderView>

#define INFO_COLUMN_KEY         0
#define INFO_COLUMN_VALUE       1
#define INFO_N_COLUMNS          2

inline void
InfoAttrModel::Record::setValue(const QString &value)
{
    (this->display = value).replace('\x1f', C(0xb7));
    this->value = value;
}

InfoAttrModel::Record::Record(const QString &value)
{
    setValue(value);
}

InfoAttrModel::InfoAttrModel(const AttributeMap &attributes, QWidget *parent) :
    QAbstractTableModel(parent)
{
    for (auto i = attributes.cbegin(), j = attributes.cend(); i != j; ++i)
        m_map.insert(i.key(), Record(*i));
}

/*
 * Model functions
 */
int
InfoAttrModel::columnCount(const QModelIndex &parent) const
{
    return INFO_N_COLUMNS;
}

int
InfoAttrModel::rowCount(const QModelIndex &parent) const
{
    return m_map.size();
}

QVariant
InfoAttrModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        switch (section) {
        case INFO_COLUMN_KEY:
            return tr("Key", "heading");
        case INFO_COLUMN_VALUE:
            return tr("Value", "heading");
        default:
            break;
        }

    return QVariant();
}

QVariant
InfoAttrModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    auto i = m_map.begin() + row;

    switch (index.column()) {
    case INFO_COLUMN_KEY:
        switch (role) {
        case Qt::DisplayRole:
            return i.key();
        }
        break;
    case INFO_COLUMN_VALUE:
        switch (role) {
        case Qt::DisplayRole:
            return i->display;
        case Qt::BackgroundRole:
            return i->animation ? i->animation->colorVariant() : QVariant();
        }
        break;
    }

    return QVariant();
}

Qt::ItemFlags
InfoAttrModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}

void
InfoAttrModel::handleAttributeChanged(const QString &key, const QString &value)
{
    auto i = m_map.find(key), j = m_map.end();
    int row;

    if (i != j) {
        i->setValue(value);
        row = std::distance(m_map.begin(), i);

        QModelIndex index = createIndex(row, INFO_COLUMN_VALUE);
        emit dataChanged(index, index);
    }
    else {
        i = m_map.lowerBound(key);
        row = std::distance(m_map.begin(), i);

        beginInsertRows(QModelIndex(), row, row);
        i = m_map.insert(i, key, Record(value));
        endInsertRows();

        // Adjust animation rows
        int nrow = row + 1;
        for (auto k = i, j = m_map.end(); ++k != j; ++nrow)
            if (k->animation)
                k->animation->setData(nrow);
    }

    if (m_visible) {
        if (!i->animation) {
            i->animation = new InfoAnimation(this, row);
            connect(i->animation, SIGNAL(animationSignal(intptr_t)),
                    SLOT(handleAnimation(intptr_t)));
        }
        i->animation->startColor(static_cast<QWidget*>(parent()));
    }
}

void
InfoAttrModel::handleAttributeRemoved(const QString &key)
{
    auto i = m_map.find(key), j = m_map.end();

    if (i != j) {
        int row = std::distance(m_map.begin(), i);

        beginRemoveRows(QModelIndex(), row, row);
        delete i->animation;
        i = m_map.erase(i);
        endRemoveRows();

        // Adjust animation rows
        for (j = m_map.end(); i != j; ++i, ++row)
            if (i->animation)
                i->animation->setData(row);
    }
}

void
InfoAttrModel::data(int row, QString &key, QString &value) const
{
    auto i = m_map.begin() + row;
    key = i.key();
    value = i->value;
}

void
InfoAttrModel::handleAnimation(intptr_t row)
{
    QModelIndex index = createIndex(row, INFO_COLUMN_VALUE);
    emit dataChanged(index, index, QVector<int>(1, Qt::BackgroundRole));
}

//
// View
//
InfoAttrView::InfoAttrView(InfoAttrModel *model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(model);
    delete m;

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAlternatingRowColors(true);

    verticalHeader()->setVisible(false);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setStretchLastSection(true);
}
