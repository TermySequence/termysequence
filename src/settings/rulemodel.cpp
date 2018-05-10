// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/defmap.h"
#include "rulemodel.h"
#include "keymap.h"

#include <QHeaderView>

#define RULE_COLUMN_KEY 0
#define RULE_COLUMN_PRIORITY 1
#define RULE_COLUMN_MODIFIERS 2
#define RULE_COLUMN_ADDITIONAL 3
#define RULE_COLUMN_TYPE 4
#define RULE_COLUMN_ACTION 5
#define RULE_N_COLUMNS 6

//
// Model
//
KeymapRuleModel::KeymapRuleModel(const TermKeymap *keymap, QObject *parent) :
    QAbstractItemModel(parent)
{
    loadRules(keymap);
}

void
KeymapRuleModel::calculateIndexes(KeymapRulelist *list, int start, int end)
{
    for (int i = start; i <= end; ++i) {
        KeymapRulelist &inner = list->at(i)->comboRulelist;

        for (int j = 0; j < inner.size(); ++j)
            inner.at(j)->parentIndex = i;

        list->at(i)->index = i;
    }
}

void
KeymapRuleModel::calculatePriorities(KeymapRulelist *list, int key, const QModelIndex &parent)
{
    int start, row, max;

    for (row = 0; row < list->size(); ++row) {
        if (list->at(row)->key == key) {
            start = row;
            goto next;
        }
    }
    return;
next:
    for (; row < list->size() && list->at(row)->key == key; ++row)
        list->at(row)->priority = row - start + 1;

    max = row - start;

    for (--row; row >= start; --row)
        list->at(row)->maxPriority = max;

    // trigger sort via key column
    QModelIndex si = index(start, RULE_COLUMN_KEY, parent);
    QModelIndex ei = index(start + max - 1, RULE_COLUMN_PRIORITY, parent);
    emit dataChanged(si, ei);
}

const QModelIndex
KeymapRuleModel::moveFirst(const QModelIndex &index_)
{
    const QModelIndex parent = index_.parent();
    int row = index_.row();
    KeymapRulelist *list;

    const auto rule = (const OrderedKeymapRule*)index_.internalPointer();

    if (parent.isValid()) {
        OrderedKeymapRule *parentRule = (OrderedKeymapRule*)parent.internalPointer();
        list = &parentRule->comboRulelist;
    } else {
        list = &m_rules;
    }

    if (rule->priority > 1) {
        int pos = row - rule->priority + 1;

        list->insert(pos, list->take(row));

        calculateIndexes(list, pos, row);
        calculatePriorities(list, rule->key, parent);

        QModelIndex si = index(pos, 0, parent);
        QModelIndex ei = index(row, RULE_N_COLUMNS - 1, parent);
        emit dataChanged(si, ei);
        row = pos;
    }
    return index(row, 0, parent);
}

const QModelIndex
KeymapRuleModel::moveUp(const QModelIndex &index_)
{
    const QModelIndex parent = index_.parent();
    int row = index_.row();
    KeymapRulelist *list;

    const auto rule = (const OrderedKeymapRule*)index_.internalPointer();

    if (parent.isValid()) {
        OrderedKeymapRule *parentRule = (OrderedKeymapRule*)parent.internalPointer();
        list = &parentRule->comboRulelist;
    } else {
        list = &m_rules;
    }

    if (rule->priority > 1) {
        int pos = row - 1;

        list->insert(pos, list->take(row));

        calculateIndexes(list, pos, row);
        calculatePriorities(list, rule->key, parent);

        QModelIndex si = index(pos, 0, parent);
        QModelIndex ei = index(row, RULE_N_COLUMNS - 1, parent);
        emit dataChanged(si, ei);
        row = pos;
    }
    return index(row, 0, parent);
}

const QModelIndex
KeymapRuleModel::moveDown(const QModelIndex &index_)
{
    const QModelIndex parent = index_.parent();
    int row = index_.row();
    KeymapRulelist *list;

    const auto rule = (const OrderedKeymapRule*)index_.internalPointer();

    if (parent.isValid()) {
        OrderedKeymapRule *parentRule = (OrderedKeymapRule*)parent.internalPointer();
        list = &parentRule->comboRulelist;
    } else {
        list = &m_rules;
    }

    if (rule->priority < rule->maxPriority) {
        int pos = row + 1;

        list->insert(pos, list->take(row));

        calculateIndexes(list, row, pos);
        calculatePriorities(list, rule->key, parent);

        QModelIndex si = index(row, 0, parent);
        QModelIndex ei = index(pos, RULE_N_COLUMNS - 1, parent);
        emit dataChanged(si, ei);
        row = pos;
    }
    return index(row, 0, parent);
}

const QModelIndex
KeymapRuleModel::moveLast(const QModelIndex &index_)
{
    const QModelIndex parent = index_.parent();
    int row = index_.row();
    KeymapRulelist *list;

    const auto rule = (const OrderedKeymapRule*)index_.internalPointer();

    if (parent.isValid()) {
        OrderedKeymapRule *parentRule = (OrderedKeymapRule*)parent.internalPointer();
        list = &parentRule->comboRulelist;
    } else {
        list = &m_rules;
    }

    if (rule->priority < rule->maxPriority) {
        int pos = row + (rule->maxPriority - rule->priority);

        list->insert(pos, list->take(row));

        calculateIndexes(list, row, pos);
        calculatePriorities(list, rule->key, parent);

        QModelIndex si = index(row, 0, parent);
        QModelIndex ei = index(pos, RULE_N_COLUMNS - 1, parent);
        emit dataChanged(si, ei);
        row = pos;
    }
    return index(row, 0, parent);
}

const QModelIndex
KeymapRuleModel::addRule(OrderedKeymapRule &rule, const QModelIndex &parent)
{
    KeymapRulelist *list;

    if (parent.isValid()) {
        OrderedKeymapRule *parentRule = (OrderedKeymapRule*)parent.internalPointer();
        list = &parentRule->comboRulelist;
        rule.parentIndex = parentRule->index;
    } else {
        list = &m_rules;
        rule.parentIndex = -1;
    }

    rule.index = 0;
    const int n = list->size();
    int row;

    for (row = 0; row < n; ++row) {
        if (list->at(row)->key == rule.key) {
            while (row < n &&
                   list->at(row)->key == rule.key &&
                   list->at(row)->priority < rule.priority)
                ++row;
            goto next;
        }
    }
    row = 0;
next:
    beginInsertRows(parent, row, row);
    list->insert(row, new OrderedKeymapRule(rule));
    calculateIndexes(list, row, n);
    endInsertRows();

    calculatePriorities(list, rule.key, parent);
    return index(row, 0, parent);
}

void
KeymapRuleModel::removeRule(const QModelIndex &index)
{
    const QModelIndex parent = index.parent();
    int row = index.row();
    KeymapRulelist *list;

    const auto rule = (const OrderedKeymapRule*)index.internalPointer();
    int key = rule->key;

    if (parent.isValid()) {
        OrderedKeymapRule *parentRule = (OrderedKeymapRule*)parent.internalPointer();
        list = &parentRule->comboRulelist;
    } else {
        list = &m_rules;
    }

    beginRemoveRows(parent, row, row);
    list->erase(row);
    calculateIndexes(list, row, list->size() - 1);
    endRemoveRows();

    calculatePriorities(list, key, parent);
}

void
KeymapRuleModel::loadRules(const TermKeymap *keymap)
{
    int index = 0;
    int key = 0;
    int priority = 0;

    beginResetModel();
    m_rules.clear();

    for (const auto &i: *keymap->rules()) {
        auto *rule = new OrderedKeymapRule(*i.second, index++);

        if (key != rule->key) {
            key = rule->key;
            priority = 0;
        }
        rule->priority = ++priority;
        m_rules.append(rule);
    }

    key = 0;
    for (auto i = m_rules.rbegin(), j = m_rules.rend(); i != j; ++i) {
        if (key != (*i)->key) {
            key = (*i)->key;
            priority = (*i)->priority;
        }
        (*i)->maxPriority = priority;
    }
    endResetModel();
}

void
KeymapRuleModel::loadDefaultRules()
{
    int index = 0;
    int key = 0;
    int priority = 0;

    beginResetModel();
    m_rules.clear();
    for (const KeymapRule *cur = g_defaultKeymap; cur->key; ++cur) {
        auto *rule = new OrderedKeymapRule(*cur, index++);
        rule->populateStrings();

        if (key != rule->key) {
            key = rule->key;
            priority = 0;
        }
        rule->priority = ++priority;
        m_rules.append(rule);
    }

    key = 0;
    for (auto i = m_rules.rbegin(), j = m_rules.rend(); i != j; ++i) {
        if (key != (*i)->key) {
            key = (*i)->key;
            priority = (*i)->priority;
        }
        (*i)->maxPriority = priority;
    }
    endResetModel();
}

/*
 * Model functions
 */
int
KeymapRuleModel::columnCount(const QModelIndex &parent) const
{
    return RULE_N_COLUMNS;
}

int
KeymapRuleModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_rules.size();
    else if (parent.column() == 0)
        return ((const OrderedKeymapRule *)parent.internalPointer())->comboRulelist.size();
    else
        return 0;
}

QVariant
KeymapRuleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        switch (section) {
        case RULE_COLUMN_KEY:
            return tr("Key/Button", "heading");
        case RULE_COLUMN_MODIFIERS:
            return tr("Modifiers", "heading");
        case RULE_COLUMN_ADDITIONAL:
            return tr("Additional Conditions", "heading");
        case RULE_COLUMN_PRIORITY:
            return tr("Priority", "heading");
        case RULE_COLUMN_TYPE:
            return tr("Action Type", "heading");
        case RULE_COLUMN_ACTION:
            return tr("Action Value", "heading");
        }

    return QVariant();
}

QVariant
KeymapRuleModel::data(const QModelIndex &index, int role) const
{
    const auto rule = (const OrderedKeymapRule*)index.internalPointer();
    if (rule)
        switch (index.column()) {
        case RULE_COLUMN_KEY:
            if (role == Qt::DisplayRole)
                return rule->keyName;
            if (role == Qt::UserRole)
                return rule->key;
            break;
        case RULE_COLUMN_MODIFIERS:
            if (role == Qt::DisplayRole)
                return rule->modifierStr;
            break;
        case RULE_COLUMN_ADDITIONAL:
            if (role == Qt::DisplayRole)
                return rule->additionalStr;
            break;
        case RULE_COLUMN_PRIORITY:
            if (role == Qt::DisplayRole)
                return rule->priority;
//	    if (role == Qt::UserRole)
//	        return rule->maxPriority;
            break;
        case RULE_COLUMN_TYPE:
            if (role == Qt::DisplayRole) {
                if (!rule->special)
                    return tr("Input");
                else if (rule->startsCombo)
                    return tr("Digraph");
                else
                    return tr("Action");
            }
            break;
        case RULE_COLUMN_ACTION:
            if (role == Qt::DisplayRole)
                return rule->startsCombo ? QVariant() : rule->outcomeStr;
            break;
        }

    return QVariant();
}

Qt::ItemFlags
KeymapRuleModel::flags(const QModelIndex &index) const
{
    switch (index.column()) {
    default:
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
}

bool
KeymapRuleModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_rules.size() > 0;
    if (parent.column())
        return false;

    const auto rule = (const OrderedKeymapRule*)parent.internalPointer();
    return rule->comboRulelist.size() > 0;
}

QModelIndex
KeymapRuleModel::parent(const QModelIndex &index) const
{
    const auto rule = (const OrderedKeymapRule*)index.internalPointer();

    if (rule->parentIndex != -1)
        return createIndex(rule->parentIndex, 0, (void *)m_rules.at(rule->parentIndex));
    else
        return QModelIndex();
}

QModelIndex
KeymapRuleModel::index(int row, int column, const QModelIndex &parent) const
{
    const KeymapRulelist &list = parent.isValid() ?
        ((const OrderedKeymapRule*)parent.internalPointer())->comboRulelist :
        m_rules;

    if (row < list.size())
        return createIndex(row, column, (void *)list.at(row));
    else
        return QModelIndex();
}

//
// Filter
//
KeymapRuleFilter::KeymapRuleFilter(KeymapRuleModel *model, QObject *parent):
    QSortFilterProxyModel(parent),
    m_model(model),
    m_searchActive(false),
    m_keyActive(false)
{
    setSourceModel(model);
}

void
KeymapRuleFilter::setSearchString(const QString &str)
{
    if (str.isEmpty()) {
        m_searchActive = false;
    }
    else {
        m_search = str;
        m_searchActive = true;
        m_keyActive = false;
    }

    invalidateFilter();
}

void
KeymapRuleFilter::setKeystroke(int key)
{
    m_key = key;
    m_keyActive = true;
    m_searchActive = false;

    invalidateFilter();
}

void
KeymapRuleFilter::resetSearch()
{
    m_searchActive = m_keyActive = false;
    invalidateFilter();
}

bool
KeymapRuleFilter::isSearchMatch(int row, const QModelIndex &parent) const
{
    QStringList buffer;
    buffer.append(m_model->data(m_model->index(row, RULE_COLUMN_KEY, parent)).toString());
    buffer.append(m_model->data(m_model->index(row, RULE_COLUMN_MODIFIERS, parent)).toString());
    buffer.append(m_model->data(m_model->index(row, RULE_COLUMN_ADDITIONAL, parent)).toString());
    buffer.append(m_model->data(m_model->index(row, RULE_COLUMN_TYPE, parent)).toString());
    buffer.append(m_model->data(m_model->index(row, RULE_COLUMN_ACTION, parent)).toString());

    return buffer.join('\n').contains(m_search, Qt::CaseInsensitive);
}

bool
KeymapRuleFilter::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    if (m_keyActive) {
        int key;

        if (parent.isValid()) {
            key = m_model->data(parent, Qt::UserRole).toInt();
        } else {
            QModelIndex index = m_model->index(row, RULE_COLUMN_KEY, parent);
            key = m_model->data(index, Qt::UserRole).toInt();
        }

        return key == m_key;
    }
    else if (m_searchActive) {
        if (isSearchMatch(row, parent))
            return true;
        else {
            QModelIndex index = m_model->index(row, 0, parent);
            int n = m_model->rowCount(index);
            for (int i = 0; i < n; ++i)
                if (isSearchMatch(i, index))
                    return true;
        }

        return false;
    }

    return true;
}

bool
KeymapRuleFilter::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (left.column() == RULE_COLUMN_KEY && right.column() == RULE_COLUMN_KEY) {
        const auto leftRule = (const OrderedKeymapRule*)left.internalPointer();
        const auto rightRule = (const OrderedKeymapRule*)right.internalPointer();

        if (leftRule->key == rightRule->key)
            return leftRule->priority > rightRule->priority;
        else
            return leftRule->key > rightRule->key;
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

//
// View
//
KeymapRuleView::KeymapRuleView(QSortFilterProxyModel *filter) :
    m_filter(filter)
{
    QItemSelectionModel *m = selectionModel();
    setModel(filter);
    delete m;

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSortingEnabled(true);
    setAlternatingRowColors(true);

    header()->setStretchLastSection(true);
    header()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

OrderedKeymapRule *
KeymapRuleView::selectedRule() const
{
    QModelIndexList indexes = selectionModel()->selectedRows(0);
    if (indexes.size() != 1)
        return nullptr;

    QModelIndex sindex = m_filter->mapToSource(indexes.at(0));
    return (OrderedKeymapRule *)sindex.internalPointer();
}

QModelIndex
KeymapRuleView::selectedIndex() const
{
    QModelIndexList indexes = selectionModel()->selectedRows(0);

    return indexes.size() == 1 ?
        m_filter->mapToSource(indexes.at(0)) :
        QModelIndex();
}
