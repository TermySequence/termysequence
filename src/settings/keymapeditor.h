// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPushButton;
class QLineEdit;
QT_END_NAMESPACE
class TermKeymap;
class KeymapRuleModel;
class KeymapRuleFilter;
class KeymapRuleView;

class KeymapEditor final: public QWidget
{
    Q_OBJECT

private:
    TermKeymap *m_keymap;

    KeymapRuleModel *m_model;
    KeymapRuleFilter *m_filter;
    KeymapRuleView *m_view;

    bool m_accepting;

    QPushButton *m_newButton;
    QPushButton *m_childButton;
    QPushButton *m_deleteButton;
    QPushButton *m_editButton;

    QPushButton *m_optionButton;
    QPushButton *m_applyButton;
    QPushButton *m_resetButton;

    QPushButton *m_firstButton;
    QPushButton *m_upButton;
    QPushButton *m_downButton;
    QPushButton *m_lastButton;

    QLineEdit *m_search;

private slots:
    void handleSelection();
    void handleResetSearch();

    void handleNewRule();
    void handleChildRule();
    void handleDeleteRule();
    void handleEditRule();

    void handleChange();
    void handleDefaults();
    void handleOptions();

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
    KeymapEditor(TermKeymap *keymap);

    QSize sizeHint() const { return QSize(1024, 800); }

    void bringUp();
};
