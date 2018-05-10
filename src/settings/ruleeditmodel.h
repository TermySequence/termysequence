// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "rule.h"

#include <QAbstractTableModel>
#include <QTableView>

#define RULEFLAG_COLUMN_NAME 0
#define RULEFLAG_COLUMN_DESCRIPTION 1
#define RULEFLAG_COLUMN_CONDITION 2
#define RULEFLAG_N_COLUMNS 3

//
// Model
//
class KeymapFlagsModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    Tsq::TermFlags m_conditions;
    Tsq::TermFlags m_mask;

    QStringList m_choices;

public:
    KeymapFlagsModel(Tsq::TermFlags conditions, Tsq::TermFlags mask, QObject *parent);

    Tsq::TermFlags conditions() const { return m_conditions; }
    Tsq::TermFlags mask() const { return m_mask; }

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);
};

//
// View
//
class KeymapFlagsView final: public QTableView
{
public:
    KeymapFlagsView(KeymapFlagsModel *model);
};
