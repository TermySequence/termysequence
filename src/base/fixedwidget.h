// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "splitwidget.h"

class FixedWidget final: public SplitWidget
{
private:
    bool m_horizontal;

    QList<SplitWidget*> m_children;
    QList<int> m_sizes;
    int m_totalSize;

    QSize m_sizeHint;

    void calculateSizeHint();
    void relayout();
    void equalize();

    void handleSplitSplit(int type, SplitWidget *caller);
    void handleSplitClose(SplitWidget *caller);
    void handleSplitAdjust(int type, SplitWidget *caller);

protected:
    void resizeEvent(QResizeEvent *event);

public:
    FixedWidget(bool horizontal, const QList<SplitWidget*> &children, const QList<int> &sizes);
    FixedWidget(bool horizontal, SplitWidget *child, SplitBase *base);

    inline SplitWidget* sibling() { return m_children.at(1); }

    QSize sizeHint() const;

    void handleSplitRequest(int type, SplitWidget *caller);
    void replaceChild(SplitWidget *child, SplitWidget *replacement);
    void saveLayout(SplitLayoutWriter &layout) const;

    void takeFocus();
};
