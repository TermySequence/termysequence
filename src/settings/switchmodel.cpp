// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/simpleitem.h"
#include "switchmodel.h"
#include "settings.h"

#include <QHeaderView>

#define SWITCH_COLUMN_TEXT0         0
#define SWITCH_COLUMN_FROMVERB      1
#define SWITCH_COLUMN_FROMPROFILE   2
#define SWITCH_COLUMN_TEXT1         3
#define SWITCH_COLUMN_VARNAME       4
#define SWITCH_COLUMN_VARVERB       5
#define SWITCH_COLUMN_VARVALUE      6
#define SWITCH_COLUMN_TEXT2         7
#define SWITCH_COLUMN_TOVERB        8
#define SWITCH_COLUMN_TOPROFILE     9
#define SWITCH_N_COLUMNS            10

//
// Model
//
SwitchRuleModel::SwitchRuleModel(const SwitchRuleset *ruleset, QObject *parent) :
    QAbstractTableModel(parent),
    m_ruleset(*ruleset),
    m_fromVerbNames(SwitchRule::getFromVerbNames()),
    m_varVerbNames(SwitchRule::getVarVerbNames()),
    m_toVerbNames(SwitchRule::getToVerbNames())
{
    connect(g_settings, SIGNAL(profilesChanged()), SLOT(resetProfiles()));
    resetProfiles();
}

void
SwitchRuleModel::resetProfiles()
{
    // new profile lists in combo boxes
    m_profileNames.clear();
    m_profileIcons.clear();
    for (auto i: g_settings->allProfiles()) {
        m_profileNames.append(i.first);
        m_profileIcons.append(i.second);
    }

    int size = m_ruleset.size();
    if (size > 0) {
        QModelIndex si = index(0, SWITCH_COLUMN_FROMPROFILE);
        QModelIndex ei = index(size - 1, SWITCH_COLUMN_FROMPROFILE);
        emit dataChanged(si, ei);
        si = index(0, SWITCH_COLUMN_TOPROFILE);
        ei = index(size - 1, SWITCH_COLUMN_TOPROFILE);
        emit dataChanged(si, ei);
    }
}

void
SwitchRuleModel::moveFirst(int start, int end)
{
    for (int i = 0; i < end - start + 1; ++i)
        m_ruleset.swap(i, start + i);

    QModelIndex si = index(0, 0);
    QModelIndex ei = index(end, SWITCH_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
SwitchRuleModel::moveUp(int start, int end)
{
    m_ruleset.move(start - 1, end);

    QModelIndex si = index(start - 1, 0);
    QModelIndex ei = index(end, SWITCH_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
SwitchRuleModel::moveDown(int start, int end)
{
    m_ruleset.move(end + 1, start);

    QModelIndex si = index(start, 0);
    QModelIndex ei = index(end + 1, SWITCH_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
SwitchRuleModel::moveLast(int start, int end)
{
    int n = end - start + 1;
    int m = m_ruleset.size() - n;
    for (int i = 0; i < n; ++i)
        m_ruleset.swap(m + i, start + i);

    QModelIndex si = index(start, 0);
    QModelIndex ei = index(m_ruleset.size() - 1, SWITCH_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
SwitchRuleModel::insertRule(int row)
{
    beginInsertRows(QModelIndex(), row, row);
    m_ruleset.insert(row);
    endInsertRows();
}

void
SwitchRuleModel::cloneRule(int row)
{
    SwitchRule rule = m_ruleset.at(row++);
    beginInsertRows(QModelIndex(), row, row);
    m_ruleset.insert(row, rule);
    endInsertRows();
}

void
SwitchRuleModel::removeRule(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_ruleset.removeAt(row);
    endRemoveRows();
}

void
SwitchRuleModel::loadRules(const SwitchRuleset *ruleset)
{
    beginResetModel();
    m_ruleset = *ruleset;
    endResetModel();
}

/*
 * Model functions
 */
int
SwitchRuleModel::columnCount(const QModelIndex &parent) const
{
    return SWITCH_N_COLUMNS;
}

int
SwitchRuleModel::rowCount(const QModelIndex &parent) const
{
    return m_ruleset.size();
}

QModelIndex
SwitchRuleModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < m_ruleset.size())
        return createIndex(row, column, (void *)&m_ruleset.at(row));
    else
        return QModelIndex();
}

QVariant
SwitchRuleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case SWITCH_COLUMN_FROMVERB:
            return tr("Current Condition", "heading");
        case SWITCH_COLUMN_FROMPROFILE:
            return tr("Current Profile", "heading");
        case SWITCH_COLUMN_VARNAME:
            return tr("Attribute Name", "heading");
        case SWITCH_COLUMN_VARVERB:
            return tr("Match Condition", "heading");
        case SWITCH_COLUMN_VARVALUE:
            return tr("Match String", "heading");
        case SWITCH_COLUMN_TOVERB:
            return tr("Switch Action", "heading");
        case SWITCH_COLUMN_TOPROFILE:
            return tr("Next Profile", "heading");
        }

    return QVariant();
}

QVariant
SwitchRuleModel::data(const QModelIndex &index, int role) const
{
    const auto *rule = (const SwitchRule *)index.internalPointer();
    if (rule)
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case SWITCH_COLUMN_TEXT0:
                return tr("If profile", "text");
            case SWITCH_COLUMN_FROMVERB:
                return tr(SwitchRule::getFromVerbName(rule->fromVerb));
            case SWITCH_COLUMN_FROMPROFILE:
                return rule->fromProfile;
            case SWITCH_COLUMN_TEXT1:
                return tr("and variable", "text");
            case SWITCH_COLUMN_VARNAME:
                return rule->varName;
            case SWITCH_COLUMN_VARVERB:
                return tr(SwitchRule::getVarVerbName(rule->varVerb));
            case SWITCH_COLUMN_VARVALUE:
                return rule->varValue;
            case SWITCH_COLUMN_TEXT2:
                return tr("then", "text");
            case SWITCH_COLUMN_TOVERB:
                return tr(SwitchRule::getToVerbName(rule->toVerb));
            case SWITCH_COLUMN_TOPROFILE:
                return rule->toProfile;
            }
            break;
        case Qt::DecorationRole:
            switch (index.column()) {
            case SWITCH_COLUMN_FROMPROFILE:
                return m_profileIcons.value(m_profileNames.indexOf(rule->fromProfile));
            case SWITCH_COLUMN_TOPROFILE:
                return m_profileIcons.value(m_profileNames.indexOf(rule->toProfile));
            }
            break;
        case Qt::EditRole:
            switch (index.column()) {
            case SWITCH_COLUMN_FROMVERB:
                return (int)rule->fromVerb;
            case SWITCH_COLUMN_FROMPROFILE:
                return m_profileNames.indexOf(rule->fromProfile);
            case SWITCH_COLUMN_VARNAME:
                return rule->varName;
            case SWITCH_COLUMN_VARVERB:
                return (int)rule->varVerb;
            case SWITCH_COLUMN_VARVALUE:
                return rule->varValue;
            case SWITCH_COLUMN_TOVERB:
                return (int)rule->toVerb;
            case SWITCH_COLUMN_TOPROFILE:
                return m_profileNames.indexOf(rule->toProfile);
            }
            break;
        case Qt::UserRole:
            switch (index.column()) {
            case SWITCH_COLUMN_FROMVERB:
                return m_fromVerbNames;
            case SWITCH_COLUMN_FROMPROFILE:
            case SWITCH_COLUMN_TOPROFILE:
                return m_profileNames;
            case SWITCH_COLUMN_VARVERB:
                return m_varVerbNames;
            case SWITCH_COLUMN_TOVERB:
                return m_toVerbNames;
            }
            break;
        case Qt::UserRole + 1:
            switch (index.column()) {
            case SWITCH_COLUMN_FROMPROFILE:
            case SWITCH_COLUMN_TOPROFILE:
                return m_profileIcons;
            }
        }

    return QVariant();
}

Qt::ItemFlags
SwitchRuleModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    case SWITCH_COLUMN_TEXT0:
    case SWITCH_COLUMN_TEXT1:
    case SWITCH_COLUMN_TEXT2:
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    default:
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable;
    }
}

bool
SwitchRuleModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    SwitchRule &rule = m_ruleset.rule(index.row());

    switch (index.column()) {
    case SWITCH_COLUMN_FROMVERB:
        rule.fromVerb = (SwitchFromVerb)value.toInt();
        break;
    case SWITCH_COLUMN_FROMPROFILE:
        rule.fromProfile = m_profileNames.value(value.toInt());
        break;
    case SWITCH_COLUMN_VARNAME:
        rule.varName = value.toString();
        break;
    case SWITCH_COLUMN_VARVERB:
        rule.varVerb = (VarVerb)value.toInt();
        break;
    case SWITCH_COLUMN_VARVALUE:
        rule.varValue = value.toString();
        break;
    case SWITCH_COLUMN_TOVERB:
        rule.toVerb = (SwitchToVerb)value.toInt();
        break;
    case SWITCH_COLUMN_TOPROFILE:
        rule.toProfile = m_profileNames.value(value.toInt());
        break;
    }

    emit dataChanged(index, index);
    return true;
}

//
// View
//
SwitchRuleView::SwitchRuleView(QAbstractTableModel *model)
{
    QItemSelectionModel *m = selectionModel();
    setModel(model);
    delete m;

    setShowGrid(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ContiguousSelection);
    setEditTriggers(QAbstractItemView::AllEditTriggers);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(SWITCH_COLUMN_TEXT0, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(SWITCH_COLUMN_TEXT1, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(SWITCH_COLUMN_TEXT2, QHeaderView::Fixed);
    resizeColumnsToContents();

    auto choiceItem = new ComboBoxItemDelegate(false, this);
    setItemDelegateForColumn(SWITCH_COLUMN_FROMVERB, choiceItem);
    setItemDelegateForColumn(SWITCH_COLUMN_VARVERB, choiceItem);
    setItemDelegateForColumn(SWITCH_COLUMN_TOVERB, choiceItem);
    choiceItem = new ComboBoxItemDelegate(true, this);
    setItemDelegateForColumn(SWITCH_COLUMN_FROMPROFILE, choiceItem);
    setItemDelegateForColumn(SWITCH_COLUMN_TOPROFILE, choiceItem);
}
