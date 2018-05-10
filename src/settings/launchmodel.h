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
class LaunchSettings;

//
// Model
//
class LauncherModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    QVector<LaunchSettings*> m_launchers;

    void startAnimation(int row);
    QVariant getTypeString(const LaunchSettings *launcher) const;
    QVariant getMountString(const LaunchSettings *launcher) const;
    QVariant getNameString(const LaunchSettings *launcher) const;

private slots:
    void handleItemAdded();
    void handleItemChanged(int row);
    void handleItemReplaced(int row);
    void handleItemRemoved(int row);
    void handleReload();
    void handleAnimation(intptr_t data);

public:
    LauncherModel(QWidget *parent);

    QModelIndex indexOf(LaunchSettings *launcher) const;

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
class LauncherView final: public QTableView
{
    Q_OBJECT

private:
    LauncherModel *m_model;
    QSortFilterProxyModel *m_filter;

    void handleClicked(const QModelIndex &index);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

signals:
    void launched();

public:
    LauncherView();

    LaunchSettings* selectedLauncher() const;
    void selectLauncher(LaunchSettings *launcher);
};
