// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "rule.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QRadioButton;
class QLineEdit;
class QComboBox;
QT_END_NAMESPACE
class KeymapFlagsModel;
class KeymapFlagsView;
class TermKeymap;

//
// Rule editor
//
class RuleEditor final: public QDialog
{
    Q_OBJECT

private:
    OrderedKeymapRule m_rule;

    KeymapFlagsModel *m_model;
    KeymapFlagsView *m_view;

    QComboBox *m_keys;

    QRadioButton *m_inputRadio;
    QRadioButton *m_actionRadio;
    QRadioButton *m_comboRadio;

    QLineEdit *m_input;
    QComboBox *m_action;

    void populateLists();
    void populateRule();

private slots:
    void handleRadioChange();
    void handleAccept();
    void handleKeystroke(int key);

public:
    RuleEditor(const OrderedKeymapRule &rule, QWidget *parent);

    OrderedKeymapRule& rule() { return m_rule; }

    QSize sizeHint() const { return QSize(960, 560); }
};

//
// Keymap options
//
class KeymapOptions final: public QDialog
{
    Q_OBJECT

private:
    TermKeymap *m_keymap;

    QLineEdit *m_text;
    QComboBox *m_alt;
    QComboBox *m_meta;

private slots:
    void handleAccept();

public:
    KeymapOptions(TermKeymap *keymap, QWidget *parent);
};
