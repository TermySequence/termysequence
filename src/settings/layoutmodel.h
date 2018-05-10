// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "termlayout.h"

#include <QAbstractTableModel>
#include <QTableView>

//
// Model
//
class LayoutModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    TermLayout &m_layout;

signals:
    void modified();

public:
    LayoutModel(TermLayout &layout, QObject *parent);

    void toggleEnabled(const QModelIndex &index);
    void toggleSeparator(const QModelIndex &index);
    void swapPosition(int pos1, int pos2);

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
class LayoutView final: public QTableView
{
    Q_OBJECT

private:
    LayoutModel *m_model;

    void handleClicked(const QModelIndex &index);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

signals:
    void modified();

public:
    LayoutView(TermLayout &layout);

    inline void reloadData() { m_model->reloadData(); }

    void moveUp(int row);
    void moveDown(int row);
};
