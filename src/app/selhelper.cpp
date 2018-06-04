// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "selhelper.h"

void
selectHelper(QItemSelectionModel *sm, const QModelIndex &index,
             QItemSelectionModel::SelectionFlags command)
{
    sm->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
    sm->select(index, command);
}

void
selectHelper(QItemSelectionModel *sm, const QItemSelection &sel,
             QItemSelectionModel::SelectionFlags command)
{
    sm->setCurrentIndex(sel.indexes().at(0), QItemSelectionModel::NoUpdate);
    sm->select(sel, command);
}
