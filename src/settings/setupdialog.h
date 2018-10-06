// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QSocketNotifier;
QT_END_NAMESPACE
class SetupOutputDialog;

class SetupDialog final: public QDialog
{
    Q_OBJECT

private:
    QCheckBox *m_systemd;
    QCheckBox *m_bash, *m_zsh;

    bool m_initial;

    int m_fd = -1;
    QSocketNotifier *m_notifier;
    SetupOutputDialog *m_dialog;

private slots:
    void handleAccept();
    void handleOutput(int fd);

public:
    SetupDialog(QWidget *parent = nullptr);
    ~SetupDialog();
};
