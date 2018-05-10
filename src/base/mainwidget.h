// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "splitbase.h"

#include <QWidget>

class TermManager;
class SplitWidget;

class MainWidget final: public QWidget, public SplitBase
{
private:
    TermManager *m_manager;

    SplitWidget *m_split;
    QWidget *m_widget;

    void handleSplitSplit(int type);

    SplitWidget *makeStackWidget(SplitLayoutReader &layout, int &pos, int type);
    SplitWidget *makeSplitWidget(SplitLayoutReader &layout, int &pos, int type);
    SplitWidget *makeWidget(SplitLayoutReader &layout, int &pos);

protected:
    void resizeEvent(QResizeEvent *event);

public:
    MainWidget(TermManager *manager, QWidget *parent);
    void populate(SplitLayoutReader &layout);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    void handleSplitRequest(int type, SplitWidget *caller);
    void replaceChild(SplitWidget *child, SplitWidget *replacement);
    void saveLayout(SplitLayoutWriter &layout) const;
};
