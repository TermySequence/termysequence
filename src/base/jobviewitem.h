// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "fontbase.h"

#include <QStyledItemDelegate>

class JobViewItem final: public QStyledItemDelegate, public DisplayIterator
{
private:
    unsigned m_displayMask;

    void paintComplex(QPainter *painter, DisplayCell &dc, const QModelIndex &index) const;

public:
    JobViewItem(unsigned displayMask, QWidget *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};
