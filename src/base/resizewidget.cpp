// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "resizewidget.h"
#include "fixedwidget.h"
#include "stackwidget.h"
#include "listener.h"
#include "manager.h"
#include "stack.h"

#include <QSplitter>

ResizeWidget::ResizeWidget(bool horiz, const QList<SplitWidget*> &children, const QList<int> &sizes) :
    SplitWidget(children.front()->stack(), nullptr),
    m_horizontal(horiz)
{
    m_splitter = new QSplitter(horiz ? Qt::Horizontal : Qt::Vertical, this);

    for (auto i: children) {
        i->setBase(this);
        m_splitter->addWidget(i);
    }

    m_splitter->setSizes(sizes);
}

ResizeWidget::ResizeWidget(bool horiz, SplitWidget *child, SplitBase *base) :
    SplitWidget(child->stack(), base),
    m_horizontal(horiz)
{
    child->setBase(this);

    TermStack *stack = new TermStack(m_manager);
    StackWidget *child2 = new StackWidget(stack, this);

    stack->setTerm(g_listener->nextAvailableTerm(m_manager));
    m_manager->addStack(stack, m_stack->pos() + 1);

    m_splitter = new QSplitter(horiz ? Qt::Horizontal : Qt::Vertical, this);
    m_splitter->addWidget(child);
    m_splitter->addWidget(child2);

    equalize();
}

void
ResizeWidget::equalize()
{
    QList<int> sizes;

    for (int i = 0; i < m_splitter->count(); ++i)
        sizes.append(1);

    m_splitter->setSizes(sizes);
}

void
ResizeWidget::resizeEvent(QResizeEvent *event)
{
    m_splitter->resize(size());
}

static int
findWidget(QSplitter *splitter, QWidget *widget)
{
    int n = splitter->count();

    for (int i = 0; i < n; ++i)
        if (widget == splitter->widget(i))
            return i;

    return 0;
}

void
ResizeWidget::handleSplitSplit(int type, SplitWidget *caller)
{
    bool horiz = (type == SPLITREQ_HRESIZE || type == SPLITREQ_HFIXED);
    bool fixed = (type == SPLITREQ_HFIXED || type == SPLITREQ_VFIXED);
    int pos = findWidget(m_splitter, caller);
    SplitWidget *focusTo;

    if (horiz == m_horizontal && !fixed) {
        TermStack *stack = new TermStack(m_manager);
        focusTo = new StackWidget(stack, this);

        stack->setTerm(g_listener->nextAvailableTerm(m_manager));
        m_manager->addStack(stack, caller->stack()->pos() + 1);

        m_splitter->insertWidget(pos + 1, focusTo);
        equalize();
    }
    else {
        focusTo = static_cast<SplitWidget*>(m_splitter->widget(pos));
        QList<int> sizes = m_splitter->sizes();
        SplitWidget *widget;

        switch (type) {
        default:
            widget = new ResizeWidget(true, focusTo, this);
            break;
        case SPLITREQ_VRESIZE:
            widget = new ResizeWidget(false, focusTo, this);
            break;
        case SPLITREQ_HFIXED:
            widget = new FixedWidget(true, focusTo, this);
            break;
        case SPLITREQ_VFIXED:
            widget = new FixedWidget(false, focusTo, this);
            break;
        case SPLITREQ_QFIXED:
            widget = new FixedWidget(false, focusTo, this);
            widget->handleSplitRequest(SPLITREQ_HFIXED, focusTo);
            widget->handleSplitRequest(SPLITREQ_HFIXED, widget->sibling());
            break;
        }

        m_splitter->insertWidget(pos, widget);
        m_splitter->setSizes(sizes);
        widget->show();
    }

    focusTo->show();
    focusTo->takeFocus();
}

void
ResizeWidget::handleSplitClose(SplitWidget *caller)
{
    QWidget *widget;
    int pos = findWidget(m_splitter, caller);

    if (m_splitter->count() == 2) {
        widget = m_splitter->widget(pos == 0);
        m_base->replaceChild(this, static_cast<SplitWidget*>(widget));
    } else {
        widget = m_splitter->widget(pos ? pos - 1 : 1);
        static_cast<SplitWidget*>(widget)->takeFocus();
        caller->hide();
        caller->deleteLater();
    }
}

void
ResizeWidget::handleSplitAdjust(int type, SplitWidget *caller)
{
    int pos = findWidget(m_splitter, caller);
    QList<int> sizes = m_splitter->sizes();
    int incoming = 0, outgoing, i;

    for (i = 0; i < sizes.size(); ++i)
        incoming += sizes.at(i);

    incoming /= VIEW_EXPAND_SECTION;
    outgoing = incoming / (sizes.size() - 1);

    if (type == SPLITREQ_SHRINK) {
        incoming = -incoming;
        outgoing = -outgoing;
    }

    for (i = 0; i < sizes.size(); ++i)
        if (i == pos)
            sizes[i] += incoming;
        else
            sizes[i] -= outgoing;

    m_splitter->setSizes(sizes);
}

void
ResizeWidget::handleSplitRequest(int type, SplitWidget *caller)
{
    switch (type) {
    case SPLITREQ_HRESIZE:
    case SPLITREQ_VRESIZE:
    case SPLITREQ_HFIXED:
    case SPLITREQ_VFIXED:
    case SPLITREQ_QFIXED:
        handleSplitSplit(type, caller);
        break;
    case SPLITREQ_CLOSE:
        handleSplitClose(caller);
        break;
    case SPLITREQ_CLOSEOTHERS:
        m_base->handleSplitRequest(type, caller);
        break;
    case SPLITREQ_EXPAND:
    case SPLITREQ_SHRINK:
        handleSplitAdjust(type, caller);
        break;
    case SPLITREQ_EQUALIZEALL:
        m_base->handleSplitRequest(type, caller);
        /* fallthru */
    case SPLITREQ_EQUALIZE:
        equalize();
        break;
    }
}

void
ResizeWidget::replaceChild(SplitWidget *child, SplitWidget *replacement)
{
    int pos = findWidget(m_splitter, child);

    replacement->setBase(this);
    m_splitter->replaceWidget(pos, replacement);
    replacement->takeFocus();

    child->deleteLater();
}

void
ResizeWidget::saveLayout(SplitLayoutWriter &layout) const
{
    layout.addResize(m_horizontal, m_splitter->sizes());

    for (int i = 0; i < m_splitter->count() && i < MAX_SPLIT_WIDGETS; ++i) {
        static_cast<SplitWidget*>(m_splitter->widget(i))->saveLayout(layout);
    }
}

void
ResizeWidget::takeFocus()
{
    static_cast<SplitWidget*>(m_splitter->widget(0))->takeFocus();
}

QSize
ResizeWidget::sizeHint() const
{
    return m_splitter->sizeHint();
}
