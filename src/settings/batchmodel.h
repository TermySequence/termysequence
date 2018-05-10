// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QAbstractTableModel>
#include <QTableView>
#include <QHash>

//
// Model class
//
class BatchModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    // The batch job contents
    struct BatchRule {
        QString connName;
        QVariant connIcon;
        QString serverId;
        QString serverName;
        QVariant serverIcon;
        int index;
    };
    QList<BatchRule*> m_ruleset;

    // Combobox list state and lists
    QList<BatchRule*> m_connList;
    QHash<QString,BatchRule*> m_connMap;
    QStringList m_connNames;
    QVariantList m_connIcons;

private slots:
    void handleServerUpdated(int index);
    void handleConnUpdated(int index);

public:
    BatchModel(const QStringList &list, QObject *parent);
    ~BatchModel();

    QStringList result() const;

    void loadRules(const QStringList &list);
    void insertRule(int row);
    void cloneRule(int row);
    void removeRule(int row);

    void moveFirst(int start, int end);
    void moveUp(int start, int end);
    void moveDown(int start, int end);
    void moveLast(int start, int end);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool setData(const QModelIndex &index, const QVariant &value, int role);
};

//
// View
//
class BatchView final: public QTableView
{
public:
    BatchView(QAbstractTableModel *model);
};
