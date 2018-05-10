// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/simpleitem.h"
#include "iconmodel.h"
#include "base/thumbicon.h"

#include <QHeaderView>

#define ICON_COLUMN_TEXT0         0
#define ICON_COLUMN_VARNAME       1
#define ICON_COLUMN_VARVERB       2
#define ICON_COLUMN_VARVALUE      3
#define ICON_COLUMN_TEXT1         4
#define ICON_COLUMN_TOICON        5
#define ICON_N_COLUMNS            6

//
// Model
//
IconRuleModel::IconRuleModel(const IconRuleset *ruleset, QObject *parent) :
    QAbstractTableModel(parent),
    m_ruleset(*ruleset),
    m_varVerbNames(IconRule::getVarVerbNames())
{
}

void
IconRuleModel::resetIcons()
{
    // new icon lists in combo boxes
    m_iconNames.clear();
    m_iconIcons.clear();
    for (auto i: ThumbIcon::getAllIcons(ThumbIcon::CommandType)) {
        m_iconNames.append(i.first);
        m_iconIcons.append(i.second);
    }

    int size = m_ruleset.size();
    if (size > 0) {
        QModelIndex si = index(0, ICON_COLUMN_TOICON);
        QModelIndex ei = index(size - 1, ICON_COLUMN_TOICON);
        emit dataChanged(si, ei);
    }
}

void
IconRuleModel::moveFirst(int start, int end)
{
    for (int i = 0; i < end - start + 1; ++i)
        m_ruleset.swap(i, start + i);

    QModelIndex si = index(0, 0);
    QModelIndex ei = index(end, ICON_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
IconRuleModel::moveUp(int start, int end)
{
    m_ruleset.move(start - 1, end);

    QModelIndex si = index(start - 1, 0);
    QModelIndex ei = index(end, ICON_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
IconRuleModel::moveDown(int start, int end)
{
    m_ruleset.move(end + 1, start);

    QModelIndex si = index(start, 0);
    QModelIndex ei = index(end + 1, ICON_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
IconRuleModel::moveLast(int start, int end)
{
    int n = end - start + 1;
    int m = m_ruleset.size() - n;
    for (int i = 0; i < n; ++i)
        m_ruleset.swap(m + i, start + i);

    QModelIndex si = index(start, 0);
    QModelIndex ei = index(m_ruleset.size() - 1, ICON_N_COLUMNS - 1);
    emit dataChanged(si, ei);
}

void
IconRuleModel::insertRule(int row)
{
    beginInsertRows(QModelIndex(), row, row);
    m_ruleset.insert(row);
    endInsertRows();
}

void
IconRuleModel::cloneRule(int row)
{
    IconRule rule = m_ruleset.at(row++);
    beginInsertRows(QModelIndex(), row, row);
    m_ruleset.insert(row, rule);
    endInsertRows();
}

void
IconRuleModel::removeRule(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_ruleset.removeAt(row);
    endRemoveRows();
}

void
IconRuleModel::setIcon(int start, int end, const QString &icon)
{
    while (start <= end)
        m_ruleset.rule(start++).toIcon = icon;

    QModelIndex si = index(start, ICON_COLUMN_TOICON);
    QModelIndex ei = index(end, ICON_COLUMN_TOICON);
    emit dataChanged(si, ei);
}

void
IconRuleModel::loadRules(const IconRuleset *ruleset)
{
    beginResetModel();
    m_ruleset = *ruleset;
    endResetModel();
}

void
IconRuleModel::loadDefaultRules()
{
    beginResetModel();
    m_ruleset.resetToDefaults();
    endResetModel();
}

/*
 * Model functions
 */
int
IconRuleModel::columnCount(const QModelIndex &parent) const
{
    return ICON_N_COLUMNS;
}

int
IconRuleModel::rowCount(const QModelIndex &parent) const
{
    return m_ruleset.size();
}

QModelIndex
IconRuleModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < m_ruleset.size())
        return createIndex(row, column, (void *)&m_ruleset.at(row));
    else
        return QModelIndex();
}

QVariant
IconRuleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case ICON_COLUMN_VARNAME:
            return tr("Attribute Name", "heading");
        case ICON_COLUMN_VARVERB:
            return tr("Match Condition", "heading");
        case ICON_COLUMN_VARVALUE:
            return tr("Match String", "heading");
        case ICON_COLUMN_TOICON:
            return tr("Next Icon", "heading");
        }

    return QVariant();
}

QVariant
IconRuleModel::data(const QModelIndex &index, int role) const
{
    const auto *rule = (const IconRule *)index.internalPointer();
    if (rule)
        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case ICON_COLUMN_TEXT0:
                return tr("If variable", "text");
            case ICON_COLUMN_VARNAME:
                return rule->varName;
            case ICON_COLUMN_VARVERB:
                return tr(IconRule::getVarVerbName(rule->varVerb));
            case ICON_COLUMN_VARVALUE:
                return rule->varValue;
            case ICON_COLUMN_TEXT1:
                return tr("then use icon", "text");
            case ICON_COLUMN_TOICON:
                return rule->toIcon;
            }
            break;
        case Qt::DecorationRole:
            if (index.column() == ICON_COLUMN_TOICON)
                return m_iconIcons.value(m_iconNames.indexOf(rule->toIcon));
            break;
        case Qt::EditRole:
            switch (index.column()) {
            case ICON_COLUMN_VARNAME:
                return rule->varName;
            case ICON_COLUMN_VARVERB:
                return (int)rule->varVerb;
            case ICON_COLUMN_VARVALUE:
                return rule->varValue;
            case ICON_COLUMN_TOICON:
                return m_iconNames.indexOf(rule->toIcon);
            }
            break;
        case Qt::UserRole:
            switch (index.column()) {
            case ICON_COLUMN_VARVERB:
                return m_varVerbNames;
            case ICON_COLUMN_TOICON:
                return m_iconNames;
            }
            break;
        case Qt::UserRole + 1:
            if (index.column() == ICON_COLUMN_TOICON)
                return m_iconIcons;
            break;
        }

    return QVariant();
}

Qt::ItemFlags
IconRuleModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    case ICON_COLUMN_TEXT0:
    case ICON_COLUMN_TEXT1:
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    default:
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable;
    }
}

bool
IconRuleModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    IconRule &rule = m_ruleset.rule(index.row());

    switch (index.column()) {
    case ICON_COLUMN_VARNAME:
        rule.varName = value.toString();
        break;
    case ICON_COLUMN_VARVERB:
        rule.varVerb = (VarVerb)value.toInt();
        break;
    case ICON_COLUMN_VARVALUE:
        rule.varValue = value.toString();
        break;
    case ICON_COLUMN_TOICON:
        rule.toIcon = m_iconNames.value(value.toInt());
        break;
    }

    emit dataChanged(index, index);
    return true;
}

//
// View
//
IconRuleView::IconRuleView(QAbstractTableModel *model)
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
    horizontalHeader()->setSectionResizeMode(ICON_COLUMN_TEXT0, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(ICON_COLUMN_TEXT1, QHeaderView::Fixed);
    resizeColumnsToContents();

    auto choiceItem = new ComboBoxItemDelegate(false, this);
    setItemDelegateForColumn(ICON_COLUMN_VARVERB, choiceItem);
    choiceItem = new ComboBoxItemDelegate(true, this);
    setItemDelegateForColumn(ICON_COLUMN_TOICON, choiceItem);
}
