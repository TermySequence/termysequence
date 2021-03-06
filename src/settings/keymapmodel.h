// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QAbstractTableModel>
#include <QTableView>
#include <QVector>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
QT_END_NAMESPACE
class TermKeymap;

//
// Model
//
class KeymapModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    QVector<TermKeymap*> m_keymaps;

    void startAnimation(int row);

private slots:
    void handleItemAdded();
    void handleItemChanged(int row);
    void handleItemRemoved(int row);
    void handleAnimation(intptr_t data);

public:
    KeymapModel(QWidget *parent);

    QModelIndex indexOf(TermKeymap *keymap) const;

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
};

//
// View
//
class KeymapView final: public QTableView
{
    Q_OBJECT

private:
    KeymapModel *m_model;
    QSortFilterProxyModel *m_filter;

    void handleClicked(const QModelIndex &index);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

signals:
    void launched();

public:
    KeymapView();

    TermKeymap* selectedKeymap() const;
    void selectKeymap(TermKeymap *keymap);
};
