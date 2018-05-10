// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "termcolors.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QSpinBox;
class QLineEdit;
QT_END_NAMESPACE
class PaletteModel;
class PaletteView;

class PaletteTab final: public QWidget
{
    Q_OBJECT

private:
    Termcolors &m_tcpal;

    PaletteModel *m_model;
    PaletteView *m_view;

    QSpinBox *m_spin;
    QLineEdit *m_text;

    void setCurrentColor(const QColor &color);

private slots:
    void handleSelection();
    void handleValueChanged(int value);
    void handleTextEdited();

    void handleColorSelect();

signals:
    void modified();

public:
    PaletteTab(Termcolors &tcpal);

    void reload();
    void bringUp();
};
