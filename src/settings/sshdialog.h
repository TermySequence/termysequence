// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "connectdialog.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
QT_END_NAMESPACE

class SshDialog final: public ConnectDialog
{
    Q_OBJECT

private:
    QLineEdit *m_host, *m_flags;
    QCheckBox *m_raw, *m_pty;

private slots:
    void handleTextChanged(const QString &text);
    void handleAccept();

public:
    SshDialog(QWidget *parent, unsigned options = 0);

    static ConnectSettings* makeConnection(const QString &arg);
};
