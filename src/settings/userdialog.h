// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "connectdialog.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

class UserDialog final: public ConnectDialog
{
    Q_OBJECT

private:
    QComboBox *m_combo;
    QLineEdit *m_user;

    static void populateInfo(int type, const QString &arg, ConnectSettings *info);

private slots:
    void handleTextChanged(const QString &text);
    void handleAccept();

public:
    UserDialog(QWidget *parent, int type, unsigned options = 0);

    static ConnectSettings* makeConnection(int type, const QString &arg);
};
