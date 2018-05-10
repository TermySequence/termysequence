// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
QT_END_NAMESPACE
class SwitchRuleset;
class SwitchRuleModel;
class SwitchRuleView;

class SwitchEditor final: public QWidget
{
    Q_OBJECT

private:
    SwitchRuleset *m_ruleset;

    SwitchRuleModel *m_model;
    SwitchRuleView *m_view;

    int m_selLow = -1, m_selHigh = -1;
    bool m_accepting;

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

    bool save();

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
    void handleRejected();

protected:
    bool event(QEvent *event);

public:
    SwitchEditor(SwitchRuleset *ruleset);

    QSize sizeHint() const { return QSize(1024, 600); }

    void bringUp();
};

extern SwitchEditor *g_switchwin;
