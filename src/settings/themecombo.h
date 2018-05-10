// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include <QComboBox>

class TermPalette;
class ThemeSettings;

class ThemeCombo final: public QComboBox
{
    Q_OBJECT

private:
    const TermPalette *m_palette;
    QMetaObject::Connection m_mocIndex;
    bool m_haveCustom;

private slots:
    void handleIndexChanged(int index);

signals:
    void themeChanged(const ThemeSettings *theme);

public:
    ThemeCombo(const TermPalette *palette);

    void updateThemes();
};
