// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "connectdialog.h"

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class BatchModel;
class BatchView;
class ConnectSettings;

class BatchDialog final: public ConnectDialog
{
    Q_OBJECT

private:
    BatchModel *m_model;
    BatchView *m_view;

    bool m_editing;
    int m_selLow = -1, m_selHigh = -1;

    QPushButton *m_insertButton;
    QPushButton *m_prependButton;
    QPushButton *m_appendButton;
    QPushButton *m_cloneButton;
    QPushButton *m_deleteButton;

    QPushButton *m_firstButton;
    QPushButton *m_upButton;
    QPushButton *m_downButton;
    QPushButton *m_lastButton;

    QPushButton *m_applyButton;
    QPushButton *m_resetButton;

private slots:
    void handleSelection();
    void handleChange();

    void handleInsertRule();
    void handlePrependRule();
    void handleAppendRule();
    void handleCloneRule();
    void handleDeleteRule();

    void handleFirst();
    void handleUp();
    void handleDown();
    void handleLast();

    void handleAccept();
    void handleApply();
    void handleReset();

public:
    BatchDialog(QWidget *parent, ConnectSettings *conn = nullptr);

    QSize sizeHint() const { return QSize(960, 600); }
};
