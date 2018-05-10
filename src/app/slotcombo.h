// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QComboBox>

class SlotCombo final: public QComboBox
{
    Q_OBJECT

private slots:
    void handleTextChanged(const QString &slot);

public:
    SlotCombo();
};
