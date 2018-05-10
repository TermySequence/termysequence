// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "stackwidget.h"
#include "arrange.h"
#include "blank.h"
#include "manager.h"
#include "term.h"
#include "server.h"
#include "stack.h"
#include "scrollport.h"
#include "modtimes.h"
#include "peek.h"

#include <QStackedWidget>
#include <QResizeEvent>

StackWidget::StackWidget(TermStack *stack, SplitBase *base) :
    SplitWidget(stack, base)
{
    // Note: destroy order matters here
    m_widgetStack = new QStackedWidget(this);
    m_stack->setParent(this);

    m_peek = new TermPeek(m_manager, this);

    connect(m_manager, SIGNAL(termRemoved(TermInstance*,TermInstance*)), SLOT(handleTermRemoved(TermInstance*)));
    connect(m_stack, SIGNAL(termChanged(TermInstance*)), SLOT(handleTermChanged(TermInstance*)));
    connect(m_stack, SIGNAL(splitRequest(int)), SLOT(forwardSplitRequest(int)));
    connect(m_stack, SIGNAL(focusRequest()), SLOT(takeFocus()));

    if (m_manager->populating()) {
        m_mocReady = connect(m_manager, SIGNAL(populated()), SLOT(handlePopulated()));
        m_mocTerm = connect(m_manager, SIGNAL(termAdded(TermInstance*)),
                            SLOT(handleTermAdded(TermInstance*)));
    }

    m_widget = m_blank = new TermBlank(m_manager);
    m_widgetStack->addWidget(m_blank);
    m_childMap.insert(nullptr, m_blank);
}

TermScrollport *
StackWidget::currentScrollport() const
{
    TermArrange *arrange = qobject_cast<TermArrange*>(m_widget);
    return arrange ? arrange->scrollport() : nullptr;
}

void
StackWidget::showPeek()
{
    m_peek->bringUp();
}

void
StackWidget::handleTermChanged(TermInstance *term)
{
    auto i = m_childMap.constFind(term);
    if (i != m_childMap.cend()) {
        m_widgetStack->setCurrentWidget(m_widget = *i);
    }
    else {
        TermArrange *arrange = new TermArrange(m_stack, term);
        m_widget = arrange;

        if (!m_stateRecords.isEmpty()) {
            auto i = m_stateRecords.constFind(term->id());
            if (i != m_stateRecords.cend()) {
                arrange->scrollport()->setWants(i->offset, i->activeJob);
                arrange->modtimes()->setWants(i->modtimeRow, i->modtime);
                m_stateRecords.erase(i);
            }
        }

        m_widgetStack->addWidget(m_widget);
        m_childMap.insert(term, m_widget);

        m_widgetStack->setCurrentWidget(m_widget);
        m_widget->show();
    }

    if (m_stack == m_manager->activeStack()) {
        auto *scrollport = term ?
            static_cast<TermArrange*>(m_widget)->scrollport() :
            nullptr;
        m_manager->setActiveTerm(term, scrollport);
    }
}

void
StackWidget::handleTermRemoved(TermInstance *term)
{
    auto i = m_childMap.constFind(term);
    if (i != m_childMap.cend())
    {
        if (m_widget == *i)
            m_widget = nullptr;

        m_widgetStack->removeWidget(*i);
        (*i)->deleteLater();

        m_childMap.erase(i);
    }
}

void
StackWidget::handleTermAdded(TermInstance *term)
{
    if (m_stateId == term->id())
        m_stack->setTerm(term);
}

void
StackWidget::handlePopulated()
{
    disconnect(m_mocTerm);
    disconnect(m_mocReady);
}

void
StackWidget::resizeEvent(QResizeEvent *event)
{
    m_widgetStack->resize(event->size());
    m_peek->resize(event->size());
    m_blank->setSize(event->size());

    for (auto i = m_childMap.cbegin(), j = m_childMap.cend(); i != j; ++i)
        if (i.key() && *i != m_widget)
            static_cast<TermArrange*>(*i)->relayout(event->size());
}

void
StackWidget::forwardSplitRequest(int type)
{
    m_base->handleSplitRequest(type, this);
}

QVector<ScrollportState>
StackWidget::stateRecords() const
{
    QVector<ScrollportState> result;

    for (auto i = m_childMap.cbegin(), j = m_childMap.cend(); i != j; ++i) {
        if (i.key()) {
            auto scrollport = static_cast<TermArrange*>(*i)->scrollport();
            auto modtimes = static_cast<TermArrange*>(*i)->modtimes();

            ScrollportState state;
            state.id = i.key()->id();
            state.offset = scrollport->lockedRow();
            state.modtimeRow = modtimes->origin();
            state.modtime = modtimes->originTime();
            state.activeJob = scrollport->activeJobId();
            result.append(state);
        }
    }

    return result;
}

void
StackWidget::saveLayout(SplitLayoutWriter &layout) const
{
    TermInstance *term = m_stack->term();

    if (term == nullptr) {
        layout.addEmptyPane();
    } else {
        TermState rec = { term->id(), term->server()->id(),
                          term->profileName(), term->server()->local() };

        layout.addPane(rec, stateRecords());
    }
}

void
StackWidget::takeFocus()
{
    if (m_widget)
        m_widget->setFocus(Qt::OtherFocusReason);
}

QSize
StackWidget::calculateTermSize(const ProfileSettings *profile) const
{
    return m_blank->calculateTermSize(profile);
}

QSize
StackWidget::sizeHint() const
{
    return m_widget->sizeHint();
}
