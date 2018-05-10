// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "termlayout.h"
#include "termcolors.h"

#include <QAbstractTableModel>
#include <QTableView>

//
// Model
//
class FillModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    TermLayout &m_layout;
    const Termcolors &m_tcpal;

public:
    FillModel(TermLayout &layout, const Termcolors &tcpal, QObject *parent);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    void reloadData();
};

//
// View
//
class FillView final: public QTableView
{
    Q_OBJECT

private:
    TermLayout &m_layout;
    const Termcolors m_tcpal;
    const QFont m_font;

    FillModel *m_model;

signals:
    void modified();
    void modelReset();

public:
    FillView(TermLayout &layout, const Termcolors &tcpal, const QFont &font);

    inline void reloadData() { m_model->reloadData(); }

    void addRule();
    void editRule();
    void deleteRule();
};
