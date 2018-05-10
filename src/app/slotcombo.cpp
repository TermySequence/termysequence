// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "app/actions.h"
#include "slotcombo.h"

SlotCombo::SlotCombo()
{
    setEditable(true);
    connect(this, SIGNAL(currentTextChanged(const QString&)),
            SLOT(handleTextChanged(const QString&)));
}

void
SlotCombo::handleTextChanged(const QString &slot)
{
    setToolTip(slot.isEmpty() ? slot : actionString(slot));
}
