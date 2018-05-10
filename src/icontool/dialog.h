// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>
#include <QSet>

class SettingsDialog final: public QDialog
{
    Q_OBJECT

private:
    QSet<int> m_sizes;

private slots:
    void handleSizeChange(bool checked);

public:
    SettingsDialog(QWidget *parent);
};
