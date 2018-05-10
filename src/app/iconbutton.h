// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "icons.h"

#include <QPushButton>
#include <QDialogButtonBox>

class HelpButton final: public QPushButton
{
public:
    HelpButton(const char *page);
};

#define IconButton(icon, text) QPushButton(QI(icon), text)
#define addHelpButton(cstr) addButton(new HelpButton(cstr), QDialogButtonBox::HelpRole)
