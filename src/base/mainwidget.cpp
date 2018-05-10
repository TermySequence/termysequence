// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/config.h"
#include "mainwidget.h"
#include "stackwidget.h"
#include "resizewidget.h"
#include "fixedwidget.h"
#include "listener.h"
#include "manager.h"
#include "stack.h"

MainWidget::MainWidget(TermManager *manager, QWidget *parent) :
    QWidget(parent),
    m_manager(manager),
    m_split(nullptr),
    m_widget(new QWidget(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void
MainWidget::populate(SplitLayoutReader &layout)
{
    int runningPos = 0;
    int focusPos = layout.focusPos();

    m_widget->deleteLater();

    m_widget = m_split = makeWidget(layout, runningPos);
    m_split->setBase(this);
    m_split->setParent(this);
    m_split->setGeometry(rect());

    m_manager->actionSwitchPane(QString::number(focusPos));
}

void
MainWidget::resizeEvent(QResizeEvent *event)
{
    m_widget->setGeometry(rect());
}

void
MainWidget::handleSplitSplit(int type)
{
    bool horiz = (type == SPLITREQ_HRESIZE || type == SPLITREQ_HFIXED);
    bool fixed = (type == SPLITREQ_HFIXED || type == SPLITREQ_VFIXED);

    SplitWidget *focusTo = m_split;

    if (type == SPLITREQ_QFIXED) {
        m_split = new FixedWidget(false, focusTo, this);
        m_split->handleSplitRequest(SPLITREQ_HFIXED, focusTo);
        m_split->handleSplitRequest(SPLITREQ_HFIXED, m_split->sibling());
    }
    else if (fixed)
        m_split = new FixedWidget(horiz, focusTo, this);
    else
        m_split = new ResizeWidget(horiz, focusTo, this);

    m_widget = m_split;
    m_split->setParent(this);
    m_split->setGeometry(rect());
    m_split->show();

    focusTo->takeFocus();
}

void
MainWidget::handleSplitRequest(int type, SplitWidget *caller)
{
    switch (type) {
    case SPLITREQ_HRESIZE:
    case SPLITREQ_VRESIZE:
    case SPLITREQ_HFIXED:
    case SPLITREQ_VFIXED:
    case SPLITREQ_QFIXED:
        handleSplitSplit(type);
        break;
    case SPLITREQ_CLOSEOTHERS:
        replaceChild(m_split, caller);
        break;
    }
}

void
MainWidget::replaceChild(SplitWidget *child, SplitWidget *replacement)
{
    if (child != replacement) {
        m_widget = m_split = replacement;
        m_split->setParent(this);
        m_split->setBase(this);
        m_split->setGeometry(rect());
        m_split->show();

        child->hide();
        child->deleteLater();
        m_split->takeFocus();
    }
}

void
MainWidget::saveLayout(SplitLayoutWriter &layout) const
{
    TermStack *stack = m_manager->activeStack();
    layout.setFocusPos(stack ? stack->pos() : 0);

    m_split->saveLayout(layout);
}

QSize
MainWidget::sizeHint() const
{
    return m_widget->sizeHint();
}

QSize
MainWidget::minimumSizeHint() const
{
    return QSize();
}

SplitWidget *
MainWidget::makeStackWidget(SplitLayoutReader &layout, int &pos, int type)
{
    TermStack *stack = new TermStack(m_manager);
    StackWidget *widget = new StackWidget(stack, nullptr);
    TermInstance *term;
    TermState termrec;
    unsigned nViewports;

    switch (type) {
    case SPLIT_LOCAL:
    case SPLIT_REMOTE:
        termrec = layout.nextTermRecord();
        widget->setStateId(termrec.termId);
        term = g_listener->findTerm(m_manager, &termrec);

        nViewports = layout.nextViewportCount();
        while (nViewports--) {
            widget->addStateRecord(layout.nextViewportRecord());
        }
        break;
    default:
        term = g_listener->nextAvailableTerm(m_manager);
        break;
    }

    stack->setTerm(term);
    m_manager->addStack(stack, pos++);

    return widget;
}

SplitWidget *
MainWidget::makeSplitWidget(SplitLayoutReader &layout, int &pos, int type)
{
    bool horizontal = true, fixed = false;
    int count = 2;

    switch (type) {
    case SPLIT_VFIXED4:
        fixed = true;
    case SPLIT_VRESIZE4:
        horizontal = false;
        count = 4;
        break;
    case SPLIT_VFIXED3:
        fixed = true;
    case SPLIT_VRESIZE3:
        horizontal = false;
        count = 3;
        break;
    case SPLIT_VFIXED2:
        fixed = true;
    case SPLIT_VRESIZE2:
        horizontal = false;
        break;
    case SPLIT_HFIXED4:
        fixed = true;
    case SPLIT_HRESIZE4:
        count = 4;
        break;
    case SPLIT_HFIXED3:
        fixed = true;
    case SPLIT_HRESIZE3:
        count = 3;
        break;
    case SPLIT_HFIXED2:
        fixed = true;
    default:
        break;
    }

    QList<int> sizes = layout.nextSizes();
    QList<SplitWidget*> children;

    while (count--)
        children.append(makeWidget(layout, pos));

    if (fixed)
        return new FixedWidget(horizontal, children, sizes);
    else
        return new ResizeWidget(horizontal, children, sizes);
}

SplitWidget *
MainWidget::makeWidget(SplitLayoutReader &layout, int &pos)
{
    int type = layout.nextType();
    switch (type) {
    case SPLIT_HRESIZE2:
    case SPLIT_HRESIZE3:
    case SPLIT_HRESIZE4:
    case SPLIT_VRESIZE2:
    case SPLIT_VRESIZE3:
    case SPLIT_VRESIZE4:
    case SPLIT_HFIXED2:
    case SPLIT_HFIXED3:
    case SPLIT_HFIXED4:
    case SPLIT_VFIXED2:
    case SPLIT_VFIXED3:
    case SPLIT_VFIXED4:
        return makeSplitWidget(layout, pos, type);
    default:
        return makeStackWidget(layout, pos, type);
    }
}
