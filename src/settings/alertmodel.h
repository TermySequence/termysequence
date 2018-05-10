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
class AlertSettings;

//
// Model
//
class AlertModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    QVector<AlertSettings*> m_alerts;

    void startAnimation(int row);
    QVariant getActionString(const AlertSettings *alert) const;

private slots:
    void handleItemAdded();
    void handleItemChanged(int row);
    void handleItemRemoved(int row);
    void handleAnimation(intptr_t data);

public:
    AlertModel(QWidget *parent);

    QModelIndex indexOf(AlertSettings *alert) const;

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
class AlertView final: public QTableView
{
    Q_OBJECT

private:
    AlertModel *m_model;
    QSortFilterProxyModel *m_filter;

    void handleClicked(const QModelIndex &index);

signals:
    void launched();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

public:
    AlertView();

    AlertSettings* selectedAlert() const;
    void selectAlert(AlertSettings *alert);
};
