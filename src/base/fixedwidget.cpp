// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "fixedwidget.h"
#include "resizewidget.h"
#include "stackwidget.h"
#include "listener.h"
#include "manager.h"
#include "stack.h"

FixedWidget::FixedWidget(bool horiz, const QList<SplitWidget*> &children, const QList<int> &sizes) :
    SplitWidget(children.front()->stack(), nullptr),
    m_horizontal(horiz),
    m_children(children),
    m_sizes(sizes),
    m_totalSize(0)
{
    int idx = 0;

    for (auto i: children) {
        i->setBase(this);
        i->setParent(this);

        m_totalSize += sizes.at(idx++);
    }

    calculateSizeHint();
}

FixedWidget::FixedWidget(bool horiz, SplitWidget *child, SplitBase *base) :
    SplitWidget(child->stack(), base),
    m_horizontal(horiz),
    m_children{child},
    m_sizes{1, 1},
    m_totalSize(2)
{
    child->setBase(this);

    TermStack *stack = new TermStack(m_manager);
    StackWidget *child2 = new StackWidget(stack, this);

    stack->setTerm(g_listener->nextAvailableTerm(m_manager));
    m_manager->addStack(stack, m_stack->pos() + 1);

    child->setParent(this);
    child2->setParent(this);

    m_children.append(child2);

    calculateSizeHint();
}

void
FixedWidget::calculateSizeHint()
{
    int w = 0, h = 0, t;

    if (m_horizontal) {
        for (auto i: qAsConst(m_children)) {
            w += i->sizeHint().width();
            t = i->sizeHint().height();
            h = (t > h) ? t : h;
        }
    } else {
        for (auto i: qAsConst(m_children)) {
            t = i->sizeHint().width();
            w = (t > w) ? t : w;
            h += i->sizeHint().height();
        }
    }

    m_sizeHint.setWidth(w);
    m_sizeHint.setHeight(h);
}

void
FixedWidget::relayout()
{
    QRect bounds;
    QList<int> newsizes;
    int totalSize = 0;

    // XXX what to do about size remainder?
    if (m_horizontal) {
        int totalWidth = width();
        bounds.setHeight(height());

        for (int i = 0; i < m_children.size(); ++i) {
            int w = totalWidth * m_sizes.at(i) / m_totalSize;
            bounds.setWidth(w);
            m_children.at(i)->setGeometry(bounds);
            bounds.translate(w, 0);
            newsizes.append(w);
            totalSize += w;
        }
    } else {
        int totalHeight = height();
        bounds.setWidth(width());

        for (int i = 0; i < m_children.size(); ++i) {
            int h = totalHeight * m_sizes.at(i) / m_totalSize;
            bounds.setHeight(h);
            m_children.at(i)->setGeometry(bounds);
            bounds.translate(0, h);
            newsizes.append(h);
            totalSize += h;
        }
    }

    m_sizes.swap(newsizes);
    m_totalSize = totalSize;
}

void
FixedWidget::resizeEvent(QResizeEvent *event)
{
    relayout();
}

void
FixedWidget::equalize()
{
    m_totalSize = m_sizes.size();

    for (int i = 0; i < m_totalSize; ++i)
        m_sizes[i] = 1;

    relayout();
}

static inline int
findWidget(const QList<SplitWidget*> &children, QWidget *widget)
{
    for (int i = 0; i < children.size(); ++i)
        if (widget == children.at(i))
            return i;

    return 0;
}

void
FixedWidget::handleSplitSplit(int type, SplitWidget *caller)
{
    bool horiz = (type == SPLITREQ_HRESIZE || type == SPLITREQ_HFIXED);
    bool fixed = (type == SPLITREQ_HFIXED || type == SPLITREQ_VFIXED);
    int pos = findWidget(m_children, caller);
    SplitWidget *focusTo;

    if (horiz == m_horizontal && fixed) {
        TermStack *stack = new TermStack(m_manager);
        focusTo = new StackWidget(stack, this);

        stack->setTerm(g_listener->nextAvailableTerm(m_manager));
        m_manager->addStack(stack, caller->stack()->pos() + 1);

        focusTo->setParent(this);
        m_children.insert(pos + 1, focusTo);
        m_sizes.append(1);
        equalize();
    }
    else {
        focusTo = caller;
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

        widget->setParent(this);
        m_children[pos] = widget;
        relayout();
        widget->show();
    }

    focusTo->show();
    focusTo->takeFocus();
}

void
FixedWidget::handleSplitClose(SplitWidget *caller)
{
    SplitWidget *widget;
    int pos = findWidget(m_children, caller);

    if (m_children.size() == 2) {
        widget = m_children.at(pos == 0);
        m_base->replaceChild(this, widget);
    } else {
        widget = m_children.at(pos ? pos - 1 : 0);
        delete m_children.at(pos);
        m_children.removeAt(pos);
        m_totalSize -= m_sizes.at(pos);
        m_sizes.removeAt(pos);
        widget->takeFocus();

        relayout();
    }
}

void
FixedWidget::handleSplitAdjust(int type, SplitWidget *caller)
{
    int pos = findWidget(m_children, caller);
    int incoming = 0, outgoing, i;

    for (i = 0; i < m_sizes.size(); ++i)
        incoming += m_sizes.at(i);

    incoming /= VIEW_EXPAND_SECTION;
    outgoing = incoming / (m_sizes.size() - 1);

    if (type == SPLITREQ_SHRINK) {
        incoming = -incoming;
        outgoing = -outgoing;
    }

    for (i = 0; i < m_sizes.size(); ++i)
        if (i == pos)
            m_sizes[i] += incoming;
        else
            m_sizes[i] -= outgoing;

    relayout();
}

void
FixedWidget::handleSplitRequest(int type, SplitWidget *caller)
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
FixedWidget::replaceChild(SplitWidget *child, SplitWidget *replacement)
{
    int pos = findWidget(m_children, child);

    m_children[pos] = replacement;
    replacement->setParent(this);
    replacement->setBase(this);
    replacement->show();

    child->hide();
    child->deleteLater();

    relayout();

    replacement->takeFocus();
}

void
FixedWidget::saveLayout(SplitLayoutWriter &layout) const
{
    layout.addFixed(m_horizontal, m_sizes);

    for (int i = 0; i < m_children.size() && i < MAX_SPLIT_WIDGETS; ++i) {
        m_children.at(i)->saveLayout(layout);
    }
}

void
FixedWidget::takeFocus()
{
    m_children.front()->takeFocus();
}

QSize
FixedWidget::sizeHint() const
{
    return m_sizeHint;
}
