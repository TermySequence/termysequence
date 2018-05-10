// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QComboBox;
QT_END_NAMESPACE
class TermKeymap;

class NewKeymapDialog final: public QDialog
{
    Q_OBJECT

private:
    QLineEdit *m_name;
    QCheckBox *m_inherit;
    QComboBox *m_combo;

private slots:
    void handleInheritChanged();

public:
    NewKeymapDialog(TermKeymap *keymap, bool cloning, QWidget *parent);

    QString name() const;
    QString parent() const;
};
