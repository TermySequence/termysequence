// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "termlayout.h"
#include "termcolors.h"

#include <QTabWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class LayoutView;
class FillView;

class LayoutTabs final: public QTabWidget
{
    Q_OBJECT

private:
    TermLayout &m_layout;

    LayoutView *m_lview;
    FillView *m_fview;

    QPushButton *m_upButton;
    QPushButton *m_downButton;

    QPushButton *m_editButton;
    QPushButton *m_deleteButton;

signals:
    void modified();

private slots:
    void handleSelect();

    void handleMoveUp();
    void handleMoveDown();

public:
    LayoutTabs(TermLayout &layout, const Termcolors &tcpal, const QFont &font);

    void setExclusiveTab(int tab);

    void reloadData();
};
