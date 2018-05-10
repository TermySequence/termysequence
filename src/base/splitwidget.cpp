// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "splitwidget.h"
#include "stack.h"

SplitWidget::SplitWidget(TermStack *stack, SplitBase *base) :
    m_manager(stack->manager()),
    m_stack(stack),
    m_base(base)
{
}

SplitWidget *
SplitWidget::sibling()
{
    return nullptr;
}
