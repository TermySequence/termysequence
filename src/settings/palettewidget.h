// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#pragma once

#include "settingwidget.h"

class TermPalette;
class ThemeCombo;
class ThemeSettings;

class PaletteWidget final: public SettingWidget
{
    Q_OBJECT

private:
    ThemeCombo *m_combo;
    TermPalette *m_scratch;

private slots:
    void handleCombo(const ThemeSettings *theme);
    void handleClicked();
    void handleOtherChanged(const char *property, const QVariant &value);

protected:
    void handleSettingChanged(const QVariant &value);

public:
    PaletteWidget(const SettingDef *def, SettingsBase *settings);
    ~PaletteWidget();
};


class PaletteWidgetFactory final: public SettingWidgetFactory
{
public:
    QWidget* createWidget(const SettingDef *def, SettingsBase *settings) const;
};
