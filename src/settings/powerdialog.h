// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "adjustdialog.h"

QT_BEGIN_NAMESPACE
class QSpinBox;
class QLabel;
QT_END_NAMESPACE

class PowerDialog final: public AdjustDialog
{
    Q_OBJECT

private:
    QSpinBox *m_spin;
    QLabel *m_label;

    unsigned m_value;

private slots:
    void handleValueChanged(int value);

    void handleAccept();
    void handleReset();

public:
    PowerDialog(TermInstance *term, TermManager *manager, QWidget *parent);

    void setValue(unsigned power);
};
