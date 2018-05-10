// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QHash>
#include <QString>

extern const QHash<QString,int> g_keyNames;

extern const QStringList& sortedKeyNames();


static inline bool isModifier(int key)
{
    return key >= 0x01000020 && key <= 0x01000023;
}

static inline bool isEscape(int key)
{
    return key == 0x01000000;
}

#define NEW_RULE_KEY_NAME "Space"
#define NEW_RULE_KEY 0x20
#define KEY_BUTTON_BIT 0x40000000
