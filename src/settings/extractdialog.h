// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
QT_END_NAMESPACE
class TermInstance;
class TermManager;

class ExtractDialog final: public QDialog
{
    Q_OBJECT

private:
    TermInstance *m_term;
    TermManager *m_manager;
    bool m_overwriteOk;

    QLineEdit *m_nameField;
    QCheckBox *m_reviewCheck;
    QCheckBox *m_switchCheck;

    void populateName();

private slots:
    void handleAccept();
    void handleDialog(int result);

public:
    ExtractDialog(TermInstance *term, TermManager *manager);
};
