// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

#include "termlayout.h"
#include "termcolors.h"

class LayoutTabs;

class LayoutDialog final: public QDialog
{
    Q_OBJECT

private:
    TermLayout m_layout;

    LayoutTabs *m_tabs;

private slots:
    void handleReset();

public:
    LayoutDialog(const TermLayout &layout, const Termcolors &tcpal,
                 const QFont &font, QWidget *parent);

    inline LayoutTabs* tabs() const { return m_tabs; }
    inline const TermLayout& layout() const { return m_layout; }

    QSize sizeHint() const { return QSize(640, 320); }
};
