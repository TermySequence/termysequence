// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "rule.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QTreeView>

class TermKeymap;

//
// Model class
//
class KeymapRuleModel final: public QAbstractItemModel
{
    Q_OBJECT

private:
    KeymapRulelist m_rules;

    void calculateIndexes(KeymapRulelist *list, int start, int end);
    void calculatePriorities(KeymapRulelist *list, int key, const QModelIndex &parent);

public:
    KeymapRuleModel(const TermKeymap *keymap, QObject *parent);

    inline const KeymapRulelist* rules() const { return &m_rules; }

    void loadRules(const TermKeymap *keymap);
    void loadDefaultRules();
    const QModelIndex addRule(OrderedKeymapRule &rule, const QModelIndex &parent = QModelIndex());
    void removeRule(const QModelIndex &index);

    const QModelIndex moveFirst(const QModelIndex &index);
    const QModelIndex moveUp(const QModelIndex &index);
    const QModelIndex moveDown(const QModelIndex &index);
    const QModelIndex moveLast(const QModelIndex &index);

    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    bool hasChildren(const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
};

//
// Filter/sort wrapper
//
class KeymapRuleFilter final: public QSortFilterProxyModel
{
    Q_OBJECT

private:
    KeymapRuleModel *m_model;

    QString m_search;
    int m_key;
    bool m_searchActive, m_keyActive;

    bool isSearchMatch(int row, const QModelIndex &parent) const;

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

public:
    KeymapRuleFilter(KeymapRuleModel *model, QObject *parent);

    inline bool filtering() const { return m_searchActive || m_keyActive; }

public slots:
    void setSearchString(const QString &str);
    void setKeystroke(int key);
    void resetSearch();
};

//
// View
//
class KeymapRuleView final: public QTreeView
{
private:
    QSortFilterProxyModel *m_filter;

public:
    KeymapRuleView(QSortFilterProxyModel *filter);

    OrderedKeymapRule* selectedRule() const;
    QModelIndex selectedIndex() const;
};
