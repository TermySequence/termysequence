// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "adjustdialog.h"
#include "termlayout.h"

class LayoutTabs;

class LayoutAdjust final: public AdjustDialog
{
    Q_OBJECT

private:
    TermLayout m_layout;
    TermLayout m_saved;

    LayoutTabs *m_tabs;

private slots:
    void handleLayout();

    void handleAccept();
    void handleRejected();
    void handleReset();

public:
    LayoutAdjust(TermInstance *term, TermManager *manager, QWidget *parent);

    QSize sizeHint() const { return QSize(640, 400); }
};
