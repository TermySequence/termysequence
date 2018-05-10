// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "termcolors.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QSpinBox;
class QLineEdit;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE
class ColorsModel;
class ColorsView;

class ColorsTab final: public QWidget
{
    Q_OBJECT

private:
    Termcolors &m_tcpal;

    ColorsModel *m_model;
    ColorsView *m_view;

    QSpinBox *m_spin;
    QLineEdit *m_text;
    QCheckBox *m_check;
    QPushButton *m_button;

    QMetaObject::Connection m_mocSpin;

    void setCurrentColor(const QColor &color);

private slots:
    void handleSelection();
    void handleValueChanged(int value);
    void handleTextEdited();
    void handleCheckBox(bool checked);

    void handleColorSelect();

signals:
    void modified();

public:
    ColorsTab(Termcolors &tcpal);

    void reload();
    void bringUp();
};
