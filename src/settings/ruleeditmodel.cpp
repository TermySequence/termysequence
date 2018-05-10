// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/attrbase.h"
#include "app/simpleitem.h"
#include "ruleeditmodel.h"
#include "rule.h"

#include <QHeaderView>

KeymapFlagsModel::KeymapFlagsModel(Tsq::TermFlags conditions, Tsq::TermFlags mask, QObject *parent):
    QAbstractTableModel(parent),
    m_conditions(conditions),
    m_mask(mask),
    m_choices(QStringList({A(""), tr("Must be true"), tr("Must be false")}))
{
}

/*
 * Model functions
 */
int
KeymapFlagsModel::columnCount(const QModelIndex &parent) const
{
    return RULEFLAG_N_COLUMNS;
}

int
KeymapFlagsModel::rowCount(const QModelIndex &parent) const
{
    return RULEFLAG_N_FLAGS;
}

QVariant
KeymapFlagsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case RULEFLAG_COLUMN_NAME:
        return tr("Condition", "heading");
    case RULEFLAG_COLUMN_DESCRIPTION:
        return tr("Description", "heading");
    case RULEFLAG_COLUMN_CONDITION:
        return tr("Requirement", "heading");
    }

    return QVariant();
}

QVariant
KeymapFlagsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    int row = index.row();

    switch (index.column()) {
    case RULEFLAG_COLUMN_NAME:
        if (role == Qt::DisplayRole)
            return tr(KeymapRule::getFlagName(row));
        break;
    case RULEFLAG_COLUMN_DESCRIPTION:
        if (role == Qt::DisplayRole)
            return tr(KeymapRule::getFlagDescription(row));
        break;
    case RULEFLAG_COLUMN_CONDITION: {
        Tsq::TermFlags flag = KeymapRule::getFlagValue(row);
        int idx = (m_mask & flag) ? (m_conditions & flag) ? 1 : 2 : 0;

        if (role == Qt::DisplayRole)
            return m_choices.at(idx);
        if (role == Qt::EditRole)
            return idx;
        if (role == Qt::UserRole)
            return m_choices;

        break;
    }
    }

    return QVariant();
}

Qt::ItemFlags
KeymapFlagsModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    case RULEFLAG_COLUMN_CONDITION:
        return Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable;
    default:
        return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
    }
}

bool
KeymapFlagsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    int row = index.row();
    Tsq::TermFlags flag = KeymapRule::getFlagValue(row);

    switch (value.toInt()) {
    case 1:
        m_mask |= flag;
        m_conditions |= flag;
        break;
    case 2:
        m_mask |= flag;
        m_conditions &= ~flag;
        break;
    default:
        m_mask &= ~flag;
        m_conditions &= ~flag;
        break;
    }

    return true;
}

//
// View
//
KeymapFlagsView::KeymapFlagsView(KeymapFlagsModel *model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(model);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    auto choiceItem = new ComboBoxItemDelegate(false, this);
    setItemDelegateForColumn(RULEFLAG_COLUMN_CONDITION, choiceItem);
}
