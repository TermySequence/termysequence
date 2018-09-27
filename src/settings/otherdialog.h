// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "connectdialog.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QSpinBox;
QT_END_NAMESPACE

class OtherDialog final: public ConnectDialog
{
    Q_OBJECT

private:
    QLineEdit *m_command;
    QCheckBox *m_raw, *m_pty;
    QSpinBox *m_keepalive;

private slots:
    void handleTextChanged(const QString &text);
    void handleAccept();

public:
    OtherDialog(QWidget *parent, unsigned options = 0);
};
