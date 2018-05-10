// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "splitwidget.h"

QT_BEGIN_NAMESPACE
class QSplitter;
QT_END_NAMESPACE

class ResizeWidget final: public SplitWidget
{
private:
    QSplitter *m_splitter;

    bool m_horizontal;

    void equalize();

    void handleSplitSplit(int type, SplitWidget *caller);
    void handleSplitClose(SplitWidget *caller);
    void handleSplitAdjust(int type, SplitWidget *caller);

protected:
    void resizeEvent(QResizeEvent *event);

public:
    ResizeWidget(bool horizontal, const QList<SplitWidget*> &children, const QList<int> &sizes);
    ResizeWidget(bool horizontal, SplitWidget *child, SplitBase *base);

    QSize sizeHint() const;

    void handleSplitRequest(int type, SplitWidget *caller);
    void replaceChild(SplitWidget *child, SplitWidget *replacement);
    void saveLayout(SplitLayoutWriter &layout) const;

    void takeFocus();
};
