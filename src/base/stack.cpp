// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "stack.h"
#include "manager.h"
#include "term.h"
#include "stackwidget.h"

TermStack::TermStack(TermManager *manager) :
    QObject(manager),
    m_manager(manager)
{
    connect(manager, &TermManager::termRemoved, this, &TermStack::handleTermRemoved);
}

TermStack::~TermStack()
{
    if (m_term) {
        m_term->removeStack(this);
        emit m_term->stacksChanged();
    }
}

void
TermStack::setParent(StackWidget *widget)
{
    QObject::setParent(m_parent = widget);
}

TermScrollport *
TermStack::currentScrollport() const
{
    return m_parent->currentScrollport();
}

void
TermStack::showPeek()
{
    m_parent->showPeek();
}

bool
TermStack::setTerm(TermInstance *term)
{
    bool rc = false;

    if (m_term != term) {
        TermInstance *prev = m_term;

        if (term)
            term->addStack(this);

        emit termChanged(m_term = term);

        if (term) {
            emit term->stacksChanged();
            rc = true;
        }
        if (prev) {
            prev->removeStack(this);
            emit prev->stacksChanged();
        }
    }
    return rc;
}

void
TermStack::handleTermRemoved(TermInstance *term, TermInstance *replacement)
{
    if (m_term == term)
        setTerm(replacement);
}

void
TermStack::setIndex(int index)
{
    if (m_index != index) {
        m_index = index;

        if (m_term) {
            m_term->addStack(this);
            emit m_term->stacksChanged();
        }

        emit indexChanged(index);
    }
}

QSize
TermStack::calculateTermSize(const ProfileSettings *profile) const
{
    return m_parent->calculateTermSize(profile);
}
