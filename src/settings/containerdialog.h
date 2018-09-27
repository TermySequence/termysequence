// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "connectdialog.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QLabel;
QT_END_NAMESPACE

class ContainerDialog final: public ConnectDialog
{
    Q_OBJECT

private:
    QComboBox *m_combo;
    QLineEdit *m_id1, *m_id2;
    QLabel *m_label1;

    static void populateInfo(int type, const QString &arg, ConnectSettings *info);

private slots:
    void handleIndexChanged();
    void handleTextChanged();
    void handleAccept();

public:
    ContainerDialog(QWidget *parent, int type, unsigned options = 0);

    static ConnectSettings* makeConnection(int type, const QString &arg);
};
