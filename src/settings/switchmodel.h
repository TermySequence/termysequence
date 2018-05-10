// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "switchrule.h"

#include <QAbstractTableModel>
#include <QTableView>

//
// Model class
//
class SwitchRuleModel final: public QAbstractTableModel
{
    Q_OBJECT

private:
    SwitchRuleset m_ruleset;

    QStringList m_profileNames;
    QVariantList m_profileIcons;

    QStringList m_fromVerbNames;
    QStringList m_varVerbNames;
    QStringList m_toVerbNames;

private slots:
    void resetProfiles();

public:
    SwitchRuleModel(const SwitchRuleset *ruleset, QObject *parent);

    inline const SwitchRuleset& ruleset() const { return m_ruleset; }

    void loadRules(const SwitchRuleset *ruleset);
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
class SwitchRuleView final: public QTableView
{
public:
    SwitchRuleView(QAbstractTableModel *model);
};
