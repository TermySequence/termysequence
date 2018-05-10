// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "iconrule.h"

#include <QAbstractTableModel>
#include <QTableView>

//
// Model class
//
class IconRuleModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    IconRuleset m_ruleset;

    QStringList m_iconNames;
    QVariantList m_iconIcons;

    QStringList m_varVerbNames;

public:
    IconRuleModel(const IconRuleset *ruleset, QObject *parent);

    inline const IconRuleset& ruleset() const { return m_ruleset; }
    void resetIcons();

    void loadRules(const IconRuleset *ruleset);
    void loadDefaultRules();
    void insertRule(int row);
    void cloneRule(int row);
    void removeRule(int row);
    void setIcon(int start, int end, const QString &icon);

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
class IconRuleView final: public QTableView
{
public:
    IconRuleView(QAbstractTableModel *model);
};
