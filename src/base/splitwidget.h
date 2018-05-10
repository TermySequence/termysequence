// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "splitbase.h"

#include <QWidget>

class TermManager;
class TermStack;

class SplitWidget: public QWidget, public SplitBase
{
protected:
    TermManager *m_manager;
    TermStack *m_stack;

    SplitBase *m_base;

public:
    SplitWidget(TermStack *stack, SplitBase *base);

    inline TermStack* stack() { return m_stack; }
    virtual SplitWidget* sibling();

    inline void setBase(SplitBase *base) { m_base = base; }
    virtual void takeFocus() = 0;
};
