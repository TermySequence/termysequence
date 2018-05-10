// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QVBoxLayout;
class QCheckBox;
QT_END_NAMESPACE
class TermManager;
class TermInstance;

class AdjustDialog: public QDialog
{
    Q_OBJECT

protected:
    QDialogButtonBox *m_buttonBox;
    QVBoxLayout *m_mainLayout;
    QCheckBox *m_allCheck;

    TermInstance *m_term;
    TermManager *m_manager;
    QString m_profileName;
    QString m_profileAction;

private slots:
    virtual void handleReset() = 0;
    virtual void handleAccept() = 0;
    virtual void handleRejected();
    void handleLink();

public:
    AdjustDialog(TermInstance *term, TermManager *manager,
                 const char *helpPage, QWidget *parent);
};
