// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QItemSelectionModel>

extern void
selectHelper(QItemSelectionModel *sm, const QModelIndex &index,
             QItemSelectionModel::SelectionFlags command);

#define doSelectIndex(view, idx, row) \
    selectHelper(view->selectionModel(), idx, row ? \
        QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows : \
        QItemSelectionModel::ClearAndSelect)

extern void
selectHelper(QItemSelectionModel *sm, const QItemSelection &sel,
             QItemSelectionModel::SelectionFlags command);

#define doSelectItems(view, sel, row) \
    selectHelper(view->selectionModel(), sel, row ? \
        QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows : \
        QItemSelectionModel::ClearAndSelect)
